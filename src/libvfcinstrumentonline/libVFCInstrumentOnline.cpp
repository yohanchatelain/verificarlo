/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2021                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#include "../../config.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#pragma GCC diagnostic pop

#include <cxxabi.h>
#include <fstream>
#include <functional>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "interflop/common/float_const.h"

// used for backtrace
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

#if LLVM_VERSION_MAJOR < 11
#define VECTOR_TYPE VectorType
#define GET_VECTOR_TYPE(ty, size) VectorType::get(ty, size)
#define CREATE_FMA_CALL(Builder, type, args)                                   \
  Builder.CreateIntrinsic(Intrinsic::fma, args)
#define CREATE_VECTOR_ELEMENT_COUNT(size) size
#else
#define VECTOR_TYPE FixedVectorType
#define GET_VECTOR_TYPE(ty, size) FixedVectorType::get(ty, size)
#define CREATE_FMA_CALL(Builder, type, args)                                   \
  Builder.CreateIntrinsic(Intrinsic::fma, type, args)
#define CREATE_VECTOR_ELEMENT_COUNT(size) ElementCount::getFixed(size)
#endif

using namespace llvm;
// VfclibInst pass command line arguments
static cl::opt<std::string>
    VfclibInstFunction("vfclibinst-function",
                       cl::desc("Only instrument given FunctionName"),
                       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstIncludeFile(
    "vfclibinst-include-file",
    cl::desc("Only instrument modules / functions in file IncludeNameFile "),
    cl::value_desc("IncludeNameFile"), cl::init(""));

static cl::opt<std::string> VfclibInstExcludeFile(
    "vfclibinst-exclude-file",
    cl::desc("Do not instrument modules / functions in file ExcludeNameFile "),
    cl::value_desc("ExcludeNameFile"), cl::init(""));

// static cl::opt<std::string>
//     VfclibInstVfcwrapper("vfclibinst-vfcwrapper-file",
//                          cl::desc("Name of the vfcwrapper IR file "),
//                          cl::value_desc("VfcwrapperIRFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<std::string>
    VfclibInstMode("vfclibinst-mode",
                   cl::desc("Instrumentation mode: up-down or sr"),
                   cl::value_desc("Mode"), cl::init("up-down"));

static cl::opt<int> VfclibSeed("vfclibinst-seed",
                               cl::desc("Seed for the random number generator"),
                               cl::value_desc("Seed"), cl::init(-1));

static cl::opt<bool>
    VfclibInstInstrumentFMA("vfclibinst-inst-fma",
                            cl::desc("Instrument floating point fma"),
                            cl::value_desc("InstrumentFMA"), cl::init(false));

/* pointer that hold the vfcwrapper Module */
// static Module *vfcwrapperM = nullptr;

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo
enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_FMA, FOP_CMP, FOP_IGNORE };

// Each instruction can be translated to a string representation
const std::string Fops2str[] = {"add", "sub", "mul",   "div",
                                "fma", "cmp", "ignore"};

/* valid floating-point type to instrument */
std::map<Type::TypeID, std::string> validTypesMap = {
    std::pair<Type::TypeID, std::string>(Type::FloatTyID, "float"),
    std::pair<Type::TypeID, std::string>(Type::DoubleTyID, "double")};

/* valid vector sizes to instrument */
const std::set<unsigned> validVectorSizes = {2, 4, 8, 16, 32, 64};

struct VfclibInst : public ModulePass {
  static char ID;

  VfclibInst() : ModulePass(ID) {}

  void print_backtrace() {
    void *buffer[100];
    int nptrs = backtrace(buffer, 100);
    char **strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
      perror("backtrace_symbols");
      exit(EXIT_FAILURE);
    }

    for (int j = 0; j < nptrs; j++)
      printf("%s\n", strings[j]);

    free(strings);
  }

  // Taken from
  // https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
  std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
    }
    return tokens;
  }

  // https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
  void findAndReplaceAll(std::string &data, std::string toSearch,
                         std::string replaceStr) {
    // Get the first occurrence
    size_t pos = data.find(toSearch);
    // Repeat till end is reached
    while (pos != std::string::npos) {
      // Replace this occurrence of Sub String
      data.replace(pos, toSearch.size(), replaceStr);
      // Get the next occurrence from the current position
      pos = data.find(toSearch, pos + replaceStr.size());
    }
  }

  void escape_regex(std::string &str) {
    findAndReplaceAll(str, ".", "\\.");
    // ECMAScript needs .* instead of * for matching any charactere
    // http://www.cplusplus.com/reference/regex/ECMAScript/
    findAndReplaceAll(str, "*", ".*");
  }

  std::string getSourceFileNameAbsPath(Module &M) {

    std::string filename = M.getSourceFileName();
    if (sys::path::is_absolute(filename))
      return filename;

    SmallString<4096> path;
    sys::fs::current_path(path);
    path.append("/" + filename);
    if (not sys::fs::make_absolute(path)) {
      return path.str().str();
    } else {
      return "";
    }
  }

  // TODO: raise a clean error if the file is not well formatted
  std::regex parseFunctionSetFile(Module &M, cl::opt<std::string> &fileName) {
    // Skip if empty fileName
    if (fileName.empty()) {
      return std::regex("");
    }

    // Open File
    std::ifstream loopstream(fileName.c_str());
    if (!loopstream.is_open()) {
      errs() << "Cannot open " << fileName << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }

    // Parse File, if module name matches, add function to FunctionSet
    int lineno = 0;
    std::string line;

    // return the absolute path of the source file
    std::string moduleName = getSourceFileNameAbsPath(M);
    moduleName = (moduleName.empty()) ? M.getModuleIdentifier() : moduleName;

    // Regex that contains all regex for each function
    std::string moduleRegex = "";

    while (std::getline(loopstream, line)) {
      lineno++;
      StringRef l = StringRef(line);

      // Ignore empty or commented lines
      if (l.startswith("#") || l.trim() == "") {
        continue;
      }
      std::pair<StringRef, StringRef> p = l.split(" ");

      if (p.second.equals("")) {
        errs() << "Syntax error in exclusion/inclusion file " << fileName << ":"
               << lineno << "\n";
        report_fatal_error("libVFCInstrument fatal error");
      } else {
        std::string mod = p.first.trim().str();
        std::string fun = p.second.trim().str();

        // If mod is not an absolute path,
        // we search any module containing mod
        if (sys::path::is_relative(mod)) {
          mod = "*" + sys::path::get_separator().str() + mod;
        }
        // If the user does not specify extension for the module
        // we match any extension
        if (not sys::path::has_extension(mod)) {
          mod += ".*";
        }

        escape_regex(mod);
        escape_regex(fun);

        if (std::regex_match(moduleName, std::regex(mod))) {
          moduleRegex += fun + "|";
        }
      }
    }

    loopstream.close();
    // Remove the extra | at the end
    if (not moduleRegex.empty()) {
      moduleRegex.pop_back();
    }
    return std::regex(moduleRegex);
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    // Parse both included and excluded function set
    std::regex includeFunctionRgx =
        parseFunctionSetFile(M, VfclibInstIncludeFile);
    std::regex excludeFunctionRgx =
        parseFunctionSetFile(M, VfclibInstExcludeFile);

    // Parse instrument single function option (--function)
    if (not VfclibInstFunction.empty()) {
      includeFunctionRgx = std::regex(VfclibInstFunction);
      excludeFunctionRgx = std::regex(".*");
    }

    // Find the list of functions to instrument
    std::vector<Function *> functions;
    for (auto &F : M.functions()) {

      const std::string &name = F.getName().str();

      // Included-list
      if (std::regex_match(name, includeFunctionRgx)) {
        functions.push_back(&F);
        continue;
      }

      // Excluded-list
      if (std::regex_match(name, excludeFunctionRgx)) {
        continue;
      }

      // If excluded-list is empty and included-list is not, we are done
      if (VfclibInstExcludeFile.empty() and not VfclibInstIncludeFile.empty()) {
        continue;
      } else {
        // Everything else is neither include-listed or exclude-listed
        functions.push_back(&F);
      }
    }

    // Do the instrumentation on selected functions
    for (auto F : functions) {
      modified |= runOnFunction(M, *F);
    }

    if (modified) {
      Function *init = getOrCreateInitRNGFunction(M);
      insertRNGInitFunction(init);
    }

    // runOnModule must return true if the pass modifies the IR
    return modified;
  }

  void insertRNGInitFunction(Function *init) {
    Module *M = init->getParent();
    Function *F = init;

    // Create the types for the global variable
    IntegerType *Int32Ty = Type::getInt32Ty(M->getContext());
    PointerType *VoidPtrTy = Type::getInt8PtrTy(M->getContext());
    FunctionType *VoidFuncTy =
        FunctionType::get(Type::getVoidTy(M->getContext()), false);
    PointerType *VoidFuncPtrTy = PointerType::get(VoidFuncTy, 0);

    // Create the new elements to be added
    ConstantInt *NewPriority = ConstantInt::get(Int32Ty, 65534);
    Function *InitFunc = getOrInsertFunction(M, "_sr_init_rng", VoidFuncTy);

    Constant *NullPtr = ConstantPointerNull::get(VoidPtrTy);

    // Create the struct type
    StructType *ElemTy = StructType::get(Int32Ty, VoidFuncPtrTy, VoidPtrTy);

    // Retrieve the existing llvm.global_ctors variable
    GlobalVariable *GlobalCtors = M->getGlobalVariable("llvm.global_ctors");

    std::vector<Constant *> Elements;

    if (GlobalCtors) {
      // Get the existing initializer
      if (ConstantArray *InitList =
              dyn_cast<ConstantArray>(GlobalCtors->getInitializer())) {
        for (unsigned i = 0; i < InitList->getNumOperands(); ++i) {
          Elements.push_back(InitList->getOperand(i));
        }
      }
    }

    // Add the new element to the list
    Constant *NewElem =
        ConstantStruct::get(ElemTy, NewPriority, InitFunc, NullPtr);
    Elements.push_back(NewElem);

    // Create the new initializer
    ArrayType *ArrayTy = ArrayType::get(ElemTy, Elements.size());
    Constant *NewInit = ConstantArray::get(ArrayTy, Elements);

    GlobalVariable *GV =
        new GlobalVariable(*M, ArrayTy, false, GlobalValue::AppendingLinkage,
                           NewInit, "llvm.global_ctors");

    if (GlobalCtors != nullptr) {
      GlobalCtors->replaceAllUsesWith(GV);
      auto name = GlobalCtors->getName();
      GlobalCtors->eraseFromParent();
      GV->setName(name);
    }
  }

  /* Check if Instruction I is a valid instruction to replace; scalar case */
  bool isValidScalarInstruction(Type *opType) {
    bool isValidType =
        validTypesMap.find(opType->getTypeID()) != validTypesMap.end();
    if (not isValidType) {
      errs() << "Unsupported operand type: " << *opType << "\n";
    }
    return isValidType;
  }

  /* Check if Instruction I is a valid instruction to replace; vector case */
  bool isValidVectorInstruction(Type *opType) {
    VectorType *vecType = static_cast<VectorType *>(opType);
    auto baseType = vecType->getScalarType();
#if LLVM_VERSION_MAJOR >= 13
    if (isa<ScalableVectorType>(vecType))
      report_fatal_error("Scalable vector type are not supported");
    auto size = ((::llvm::FixedVectorType *)vecType)->getNumElements();
#else
    auto size = vecType->getNumElements();
#endif
    bool isValidSize = validVectorSizes.find(size) != validVectorSizes.end();
    if (not isValidSize) {
      errs() << "Unsuported vector size: " << size << "\n";
      return false;
    }
    return isValidScalarInstruction(baseType);
  }

  /* Check if Instruction I is a valid instruction to replace */
  bool isValidInstruction(Instruction *I) {
    Type *opType = I->getOperand(0)->getType();
    if (opType->isVectorTy()) {
      return isValidVectorInstruction(opType);
    } else {
      return isValidScalarInstruction(opType);
    }
  }

  bool runOnFunction(Module &M, Function &F) {
    if (VfclibInstVerbose) {
      errs() << "In Function: ";
      errs().write_escaped(F.getName().str()) << '\n';
    }

    bool modified = false;

    for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
      modified |= runOnBasicBlock(M, *bi);
    }
    return modified;
  }

  Value *insertRNGdouble01Call(IRBuilder<> &Builder, Instruction *I) {
    Module *M = I->getModule();
    std::string randName = "get_rand_double01";
    Type *voidTy = Type::getVoidTy(I->getContext());
    Type *doubleTy = Type::getDoubleTy(I->getContext());
    FunctionType *funcType = FunctionType::get(doubleTy, {}, false);
#if LLVM_VERSION_MAJOR < 9
    Constant *RNG = M->getOrInsertFunction(randName, funcType);
#else
    FunctionCallee RNG = M->getOrInsertFunction(randName, funcType);
#endif
    Instruction *call = Builder.CreateCall(RNG);
    return call;
  }

  Type *getFPAsIntType(Type *type) {
    if (type->isFloatTy()) {
      return Type::getInt32Ty(type->getContext());
    } else if (type->isDoubleTy()) {
      return Type::getInt64Ty(type->getContext());
    } else {
      const std::string function_name = __func__;
      errs() << "[" + function_name + "] " + "Unsupported type: " << *type
             << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Type *getFPAsUIntType(Type *type) {
    if (type->isFloatTy()) {
      return Type::getInt32Ty(type->getContext());
    } else if (type->isDoubleTy()) {
      return Type::getInt64Ty(type->getContext());
    } else {
      const std::string function_name = __func__;
      errs() << "[" + function_name + "] " + "Unsupported type: " << *type
             << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Constant *getUIntValue(Type *type, uint64_t value) {
    Type *intTy = getFPAsIntType(type);
    if (type->isFloatTy() or type->isDoubleTy()) {
      return ConstantInt::get(intTy, value);
    } else {
      const std::string function_name = __func__;
      errs() << "[" + function_name + "] " + "Unsupported type: " << *type
             << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Constant *getIntValue(Type *type, uint64_t value) {
    Type *intTy = getFPAsIntType(type);
    if (type->isFloatTy() or type->isDoubleTy()) {
      return ConstantInt::get(intTy, value, true);
    } else {
      const std::string function_name = __func__;
      errs() << "[" + function_name + "] " + "Unsupported type: " << *type
             << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  std::string sr_hook_name(Instruction &I) {
    switch (I.getOpcode()) {
    case Instruction::FAdd:
      return "add2";
    case Instruction::FSub:
      // In LLVM IR the FSub instruction is used to represent FNeg
      return "sub2";
    case Instruction::FMul:
      return "mul2";
    case Instruction::FDiv:
      return "div2";
    default:
      errs() << "Unsupported opcode: " << I.getOpcodeName() << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  std::string getTypeName(Type *type) {

    if (type->isVectorTy()) {
      VectorType *VT = dyn_cast<VectorType>(type);
      type = VT->getElementType();
    }

    switch (type->getTypeID()) {
    case Type::VoidTyID:
      return "void";
    case Type::HalfTyID:
      return "half";
    case Type::FloatTyID:
      return "float";
    case Type::DoubleTyID:
      return "double";
    case Type::X86_FP80TyID:
      return "x86_fp80";
    case Type::FP128TyID:
      return "fp128";
    case Type::PPC_FP128TyID:
      return "ppc_fp128";
    case Type::IntegerTyID:
      return "integer";
    case Type::FunctionTyID:
      return "function";
    case Type::StructTyID:
      return "struct";
    case Type::ArrayTyID:
      return "array";
    case Type::PointerTyID:
      return "pointer";
    default:
      return "unknown";
    }
  }

  Value *insertRandUint64Call(IRBuilder<> &Builder, Instruction *I) {
    Module *M = I->getModule();
    Type *uint64Ty = Type::getInt64Ty(M->getContext());

    // Get the global variables, set true to search for local variables
    GlobalVariable *rng_state0 = M->getGlobalVariable("rng_state.0", true);
    GlobalVariable *rng_state1 = M->getGlobalVariable("rng_state.1", true);

    if (rng_state0 == nullptr) {
      rng_state0 = new GlobalVariable(
          *M,
          /* type */ uint64Ty,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ ConstantInt::get(uint64Ty, 0),
          /* name */ "rng_state.0",
          /* insertbefore */ nullptr,
          /* threadmode */ GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }
    if (rng_state1 == nullptr) {
      rng_state1 = new GlobalVariable(
          *M,
          /* type */ uint64Ty,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ ConstantInt::get(uint64Ty, 0),
          /* name */ "rng_state.1",
          /* insertbefore */ nullptr,
          /* threadmode */ GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }

    // Get rand uint64
    Value *rng_state0_load = Builder.CreateLoad(uint64Ty, rng_state0);
    Value *rng_state1_load = Builder.CreateLoad(uint64Ty, rng_state1);
    Value *add = Builder.CreateAdd(rng_state1_load, rng_state0_load);
    Value *shl = Builder.CreateShl(add, 17);
    Value *lshr = Builder.CreateLShr(add, 47);
    Value *add2 = Builder.CreateAdd(lshr, rng_state0_load);
    Value *or2 = Builder.CreateOr(add2, shl);
    Value *xor2 = Builder.CreateXor(rng_state1_load, rng_state0_load);
    Value *shl2 = Builder.CreateShl(rng_state0_load, 49);
    Value *lshr2 = Builder.CreateLShr(rng_state0_load, 15);
    Value *xor3 = Builder.CreateXor(xor2, lshr2);
    Value *shl3 = Builder.CreateShl(xor2, 21);
    Value *xor4 = Builder.CreateXor(xor3, shl3);
    Value *or3 = Builder.CreateOr(xor4, shl2, "rand_uint64");
    Builder.CreateStore(or3, rng_state0);
#if LLVM_VERSION_MAJOR < 9
    std::vector<Value *> args = {xor2, xor2, Builder.getInt64(28)};
    Value *fshl = Builder.CreateIntrinsic(Intrinsic::fshl, args);
#else
    Value *fshl = Builder.CreateIntrinsic(Intrinsic::fshl, {xor2->getType()},
                                          {xor2, xor2, Builder.getInt64(28)});
#endif
    Builder.CreateStore(fshl, rng_state1);
    return or3;
  }

  // start get_rand_uint64
  //   %3 = load i64, i64* @rng_state.0, align 16, !tbaa !5
  //   %4 = load i64, i64* @rng_state.1, align 16, !tbaa !5
  //   %5 = add i64 %4, %3
  //   %6 = shl i64 %5, 17
  //   %7 = lshr i64 %5, 47
  //   %8 = add i64 %7, %3
  //   %9 = or i64 %8, %6
  //   %10 = xor i64 %4, %3
  //   %11 = shl i64 %3, 49
  //   %12 = lshr i64 %3, 15
  //   %13 = xor i64 %10, %12
  //   %14 = shl i64 %10, 21
  //   %15 = xor i64 %13, %14
  //   %16 = or i64 %15, %11
  //   store i64 %16, i64* @rng_state.0, align 16, !tbaa !5
  //   %17 = call i64 @llvm.fshl.i64(i64 %10, i64 %10, i64 28) #12
  //   store i64 %17, i64* @rng_state.1, align 16, !tbaa !5
  // end get_rand_uint64
  // start get_rand_double01
  //   %18 = lshr i64 %9, 12
  //   %19 = or i64 %18, 4607182418800017408
  //   %20 = bitcast i64 %19 to double
  //   %21 = fadd double %20, -1.000000e+00
  // end get_rand_double01
  Value *insertRandDouble01Call(IRBuilder<> &Builder, Instruction *I) {
    Module *M = I->getModule();
    Type *uint64Ty = Type::getInt64Ty(M->getContext());

    // Get the global variables, set true to search for local variables
    GlobalVariable *rng_state0 = M->getGlobalVariable("rng_state.0", true);
    GlobalVariable *rng_state1 = M->getGlobalVariable("rng_state.1", true);

    if (rng_state0 == nullptr) {
      rng_state0 = new GlobalVariable(
          *M,
          /* type */ uint64Ty,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ ConstantInt::get(uint64Ty, 0),
          /* name */ "rng_state.0",
          /* insertbefore */ nullptr,
          /* threadmode */ GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }
    if (rng_state1 == nullptr) {
      rng_state1 = new GlobalVariable(
          *M,
          /* type */ uint64Ty,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ ConstantInt::get(uint64Ty, 0),
          /* name */ "rng_state.1",
          /* insertbefore */ nullptr,
          /* threadmode */ GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }

    // Get rand uint64
    Value *rng_state0_load = Builder.CreateLoad(uint64Ty, rng_state0);
    Value *rng_state1_load = Builder.CreateLoad(uint64Ty, rng_state1);
    Value *add = Builder.CreateAdd(rng_state1_load, rng_state0_load);
    Value *shl = Builder.CreateShl(add, 17);
    Value *lshr = Builder.CreateLShr(add, 47);
    Value *add2 = Builder.CreateAdd(lshr, rng_state0_load);
    Value *or2 = Builder.CreateOr(add2, shl);
    Value *xor2 = Builder.CreateXor(rng_state1_load, rng_state0_load);
    Value *shl2 = Builder.CreateShl(rng_state0_load, 49);
    Value *lshr2 = Builder.CreateLShr(rng_state0_load, 15);
    Value *xor3 = Builder.CreateXor(xor2, lshr2);
    Value *shl3 = Builder.CreateShl(xor2, 21);
    Value *xor4 = Builder.CreateXor(xor3, shl3);
    Value *or3 = Builder.CreateOr(xor4, shl2);
    Builder.CreateStore(or3, rng_state0);
#if LLVM_VERSION_MAJOR < 9
    std::vector<Value *> args = {xor2, xor2, Builder.getInt64(28)};
    Value *fshl = Builder.CreateIntrinsic(Intrinsic::fshl, args);
#else
    Value *fshl = Builder.CreateIntrinsic(Intrinsic::fshl, {xor2->getType()},
                                          {xor2, xor2, Builder.getInt64(28)});
#endif
    Builder.CreateStore(fshl, rng_state1);

    // Get rand double between 0 and 1
    Value *lshr3 = Builder.CreateLShr(or3, 12);
    Value *or4 = Builder.CreateOr(lshr3, Builder.getInt64(4607182418800017408));
    Type *bitCastTy = Type::getDoubleTy(M->getContext());
    Value *bitcast = Builder.CreateBitCast(or4, bitCastTy);
    Value *mone = ConstantFP::get(Type::getDoubleTy(M->getContext()), -1.0);
    Value *fadd = Builder.CreateFAdd(bitcast, mone);
    return fadd;
  }

  Value *insertVectorizeRandDouble01Call(IRBuilder<> &Builder, Instruction *I) {
    VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
    int size = VT->getNumElements();
    // create undef value with the same type as the vector
    Value *rand_double01 = UndefValue::get(VT);
    std::vector<Value *> elementsVec;
    for (int i = 0; i < size; i++) {
      elementsVec.push_back(insertRandDouble01Call(Builder, I));
      if (VT->getElementType()->isFloatTy()) {
        elementsVec[i] =
            Builder.CreateFPCast(elementsVec[i], VT->getElementType());
      }
      rand_double01 = Builder.CreateInsertElement(rand_double01, elementsVec[i],
                                                  Builder.getInt32(i),
                                                  "insert" + std::to_string(i));
    }
    return rand_double01;
  }

  Value *insertVectorizeRandUint64Call(IRBuilder<> &Builder, Instruction *I) {
    VECTOR_TYPE *fpVT = dyn_cast<VECTOR_TYPE>(I->getType());
    int size = fpVT->getNumElements();
    VECTOR_TYPE *iVT = GET_VECTOR_TYPE(Type::getInt64Ty(I->getContext()), size);
    // create undef value with the same type as the vector
    Value *rand_uint64 = UndefValue::get(iVT);
    std::vector<Value *> elementsVec;
    for (int i = 0; i < size; i++) {
      elementsVec.push_back(insertRandUint64Call(Builder, I));
      rand_uint64 = Builder.CreateInsertElement(rand_uint64, elementsVec[i],
                                                Builder.getInt32(i),
                                                "insert" + std::to_string(i));
    }
    return rand_uint64;
  }

  double c99hextodouble(const char *hex) {
    char *end;
    double d = strtod(hex, &end);
    if (*end != '\0') {
      fprintf(stderr, "Invalid hex string: %s\n", hex);
      exit(1);
    }
    return d;
  }

  Function *createFunction(FunctionType *functionType,
                           GlobalValue::LinkageTypes linkage, Module &M,
                           const std::string &name) {
#if LLVM_VERSION_MAJOR < 9

    return Function::Create(functionType, Function::ExternalLinkage, name, &M);
#else
    return Function::Create(functionType, Function::ExternalLinkage, name, M);
#endif
  }

  Function *getOrCreateSyscallFunction(Module &M) {
    Type *int64Ty = Type::getInt64Ty(M.getContext());
    Function *syscallF = M.getFunction("syscall");
    if (syscallF == nullptr) {
      FunctionType *syscallTy = FunctionType::get(int64Ty, {int64Ty}, true);
      syscallF =
          createFunction(syscallTy, GlobalValue::ExternalLinkage, M, "syscall");
    }
    return syscallF;
  }

  Function *getOrInsertFunction(Module &M, StringRef name,
                                FunctionType *functionType) {
#if LLVM_VERSION_MAJOR < 9
    Constant *cst = M.getOrInsertFunction(name, functionType);
    Function *function = dyn_cast<Function>(cst);
#else
    FunctionCallee callee = M.getOrInsertFunction(name, functionType);
    Function *function = dyn_cast<Function>(callee.getCallee());
#endif
    return function;
  }

  Function *getOrInsertFunction(Module *M, StringRef name,
                                FunctionType *functionType) {
#if LLVM_VERSION_MAJOR < 9
    Constant *cst = M->getOrInsertFunction(name, functionType);
    Function *function = dyn_cast<Function>(cst);
#else
    FunctionCallee callee = M->getOrInsertFunction(name, functionType);
    Function *function = dyn_cast<Function>(callee.getCallee());
#endif
    return function;
  }

  Function *getOrInsertGettimeofdayFunction(Module &M, IRBuilder<> &Builder,
                                            StructType *TimevalTy,
                                            StructType *TimezoneTy) {
    Type *int64Ty = Type::getInt64Ty(Builder.getContext());
    Type *int8PtrTy = Type::getInt8PtrTy(Builder.getContext());

    // retrieve the struct timeval and timezone type

    std::vector<Type *> typeGetTimeOfDay = {PointerType::getUnqual(TimevalTy),
                                            PointerType::getUnqual(TimezoneTy)};

    Type *int32Ty = Type::getInt32Ty(Builder.getContext());
    FunctionType *GettimeofdayFuncType =
        FunctionType::get(int32Ty, typeGetTimeOfDay, false);

    Function *GettimeofdayFunc =
        getOrInsertFunction(M, "gettimeofday", GettimeofdayFuncType);

    return GettimeofdayFunc;
  }

  void insertGettimeofdayCall(Module &M, IRBuilder<> &Builder,
                              StructType *TimevalTy, StructType *TimezoneTy,
                              Value *AllocaTimeval) {
    Function *GettimeofdayFunc =
        getOrInsertGettimeofdayFunction(M, Builder, TimevalTy, TimezoneTy);

    bool bySyscall = false;
    if (GettimeofdayFunc == nullptr) {
      GettimeofdayFunc = getOrCreateSyscallFunction(M);
      bySyscall = true;
    }

    Type *int64Ty = Type::getInt64Ty(Builder.getContext());
    Type *int8PtrTy = Type::getInt8PtrTy(Builder.getContext());

    Value *BitcastTimeval =
        Builder.CreateBitCast(AllocaTimeval, int8PtrTy, "bitcast_timeval");

    Constant *nullValue = Constant::getNullValue(TimezoneTy->getPointerTo());

    if (bySyscall) {
      Constant *gettimeofdaySyscallId =
          ConstantInt::get(int64Ty, SYS_gettimeofday);
      std::vector<Value *> args = {gettimeofdaySyscallId, AllocaTimeval,
                                   nullValue};
      Value *Syscall = Builder.CreateCall(GettimeofdayFunc, args);
    } else {
      std::vector<Value *> args = {AllocaTimeval, nullValue};
      Value *Gettimeofday = Builder.CreateCall(GettimeofdayFunc, args);
    }
  }

  GlobalVariable *getGVAlreadyInitialized(Module &M, IRBuilder<> &Builder) {
    Type *boolTy = Type::getInt1Ty(Builder.getContext());
    Constant *falseConst = ConstantInt::get(boolTy, 0);
    GlobalVariable *already_initialized =
        M.getGlobalVariable("already_initialized");
    if (already_initialized == nullptr) {
      already_initialized = new GlobalVariable(
          M,
          /* type */ boolTy,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */
          falseConst,
          /* name */ "already_initialized",
          /* insertbefore */ nullptr,
          /* threadmode */
          GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }
    return already_initialized;
  }

  GlobalVariable *getGVRNGState(Module &M, IRBuilder<> &Builder,
                                const std::string &name) {
    Type *int64Ty = Type::getInt64Ty(Builder.getContext());
    Constant *zero = ConstantInt::get(int64Ty, 0);
    GlobalVariable *rng_state = M.getGlobalVariable(name, true);
    if (rng_state == nullptr) {
      rng_state = new GlobalVariable(
          M,
          /* type */ int64Ty,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ zero,
          /* name */ name,
          /* insertbefore */ nullptr,
          /* threadmode */
          GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }
    return rng_state;
  }

  // clang-format off
  // start init
  // ; Function Attrs: nounwind uwtable
  // define internal void @init() #3 {
  //   %1 = alloca %struct.timeval, align 8
  //   %2 = load i1, i1* @already_initialized, align 1
  //   br i1 %2, label %22, label %3
  // 3:                                                ; preds = %0
  //   store i1 true, i1* @already_initialized,
  // align 1
  //   %4 = bitcast %struct.timeval* %1 to i8*
  //   call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %4) #12
  //   %5 = call i32 @gettimeofday(%struct.timeval* noundef nonnull %1, i8*noundef null) #12 
  //   %6 = getelementptr inbounds %struct.timeval,%struct.timeval* %1, i64 0, i32 0 
  //   %7 = load i64, i64* %6, align 8, !tbaa !9 
  //   %8 = getelementptr inbounds %struct.timeval, %struct.timeval* %1, i64 0, i32 1 
  //   %9 = load i64, i64* %8, align 8, !tbaa !11 %10 = xor i64 %9, %7
  //   %11 = tail call i64 (i64, ...) @syscall(i64 noundef 186) #12
  //   %12 = xor i64 %10, %11
  //   %13 = add i64 %12, -7046029254386353131
  //   %14 = lshr i64 %13, 30
  //   %15 = xor i64 %14, %13
  //   %16 = mul i64 %15, -4658895280553007687
  //   %17 = lshr i64 %16, 27
  //   %18 = xor i64 %17, %16
  //   %19 = mul i64 %18, -7723592293110705685
  //   %20 = lshr i64 %19, 31
  //   %21 = xor i64 %20, %19
  //   store i64 %21, i64* @rng_state.0, align 16, !tbaa !5
  //   store i64 %21, i64* @rng_state.1, align 16, !tbaa !5
  //   call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %4) #12
  //   br label %22
  // 22:                                               ; preds = %0, %3
  //   ret void
  // }
  // clang-format on
  Function *getOrCreateInitRNGFunction(Module &M) {
    IRBuilder<> Builder(M.getContext());

    const std::string function_name = "_sr_init_rng";
    Function *function = M.getFunction(function_name);

    if (function == nullptr or function->empty()) {
      Type *voidTy = Type::getVoidTy(Builder.getContext());
      FunctionType *funcType = FunctionType::get(voidTy, {}, false);

      if (function == nullptr) {
        function = createFunction(funcType, GlobalValue::InternalLinkage, M,
                                  "_sr_init_rng");
      }

      BasicBlock *BB =
          BasicBlock::Create(Builder.getContext(), "entry", function);
      Builder.SetInsertPoint(BB);

      GlobalVariable *already_initialized = getGVAlreadyInitialized(M, Builder);

      Type *boolTy = Type::getInt1Ty(Builder.getContext());
      Value *already_initialized_load =
          Builder.CreateLoad(boolTy, already_initialized);
      BasicBlock *initBB =
          BasicBlock::Create(Builder.getContext(), "init", function);
      BasicBlock *retBB =
          BasicBlock::Create(Builder.getContext(), "ret", function);

      Builder.CreateCondBr(already_initialized_load, retBB, initBB);

      Builder.SetInsertPoint(initBB);
      Value *store = Builder.CreateStore(
          ConstantInt::get(Type::getInt1Ty(Builder.getContext()), 1),
          already_initialized);

      Type *int8PtrTy = Type::getInt8PtrTy(Builder.getContext());
      Type *int32Ty = Type::getInt32Ty(Builder.getContext());
      Type *int64Ty = Type::getInt64Ty(Builder.getContext());
      Value *seed1 = nullptr, *seed2 = nullptr;

      if (VfclibSeed == -1) {
        StructType *TimevalTy =
            StructType::create(Builder.getContext(), "struct.timeval");
        std::vector<Type *> Elements(2, int64Ty);
        TimevalTy->setBody(Elements, /*isPacked=*/false);

        StructType *TimezoneTy =
            StructType::create(Builder.getContext(), "struct.timezone");
        TimevalTy->setBody(Elements, /*isPacked=*/false);

        AllocaInst *AllocaTimeval =
            Builder.CreateAlloca(TimevalTy, nullptr, "timeval");
        insertGettimeofdayCall(M, Builder, TimevalTy, TimezoneTy,
                               AllocaTimeval);
        // Load timeval->tv_sec
        Value *TvSecPtr =
            Builder.CreateStructGEP(TimevalTy, AllocaTimeval, 0, "tv_sec_ptr");
        seed1 = Builder.CreateLoad(int64Ty, TvSecPtr, "tv_sec");

        // Load timeval->tv_usec
        Value *TvUsecPtr =
            Builder.CreateStructGEP(TimevalTy, AllocaTimeval, 1, "tv_usec_ptr");
        seed2 = Builder.CreateLoad(int64Ty, TvUsecPtr, "tv_usec");

      } else {
        seed1 = ConstantInt::get(int64Ty, VfclibSeed);
        seed2 = ConstantInt::get(int64Ty, VfclibSeed + 32);
      }
      Value *xor1 = Builder.CreateXor(seed2, seed1);

      Function *syscallF = getOrCreateSyscallFunction(M);

      Constant *gettidSyscallId = ConstantInt::get(int64Ty, SYS_gettid);
      Value *syscall = Builder.CreateCall(syscallF, gettidSyscallId);
      Value *xor2 = Builder.CreateXor(xor1, syscall);
      Constant *nextSeedCst1 = ConstantInt::get(int64Ty, -7046029254386353131);
      Value *add = Builder.CreateAdd(xor2, nextSeedCst1);
      Value *lshr1 = Builder.CreateLShr(add, 30);
      Value *xor3 = Builder.CreateXor(lshr1, add);
      Constant *nextSeedCst2 = ConstantInt::get(int64Ty, -4658895280553007687);
      Value *mul1 = Builder.CreateMul(xor3, nextSeedCst2);
      Value *lshr2 = Builder.CreateLShr(mul1, 27);
      Value *xor4 = Builder.CreateXor(lshr2, mul1);
      Constant *nextSeedCst3 = ConstantInt::get(int64Ty, -7723592293110705685);
      Value *mul2 = Builder.CreateMul(xor4, nextSeedCst3);
      Value *lshr3 = Builder.CreateLShr(mul2, 31);
      Value *xor5 = Builder.CreateXor(lshr3, mul2);
      GlobalVariable *rng_state0 = getGVRNGState(M, Builder, "rng_state.0");
      Builder.CreateStore(xor5, rng_state0);
      GlobalVariable *rng_state1 = getGVRNGState(M, Builder, "rng_state.1");
      Builder.CreateStore(xor5, rng_state1);
      Builder.CreateBr(retBB);
      Builder.SetInsertPoint(retBB);
      Builder.CreateRetVoid();
      Builder.ClearInsertionPoint();
    }
    return function;
  }

  // clang-format off
  // %23 = fsub double %22, %0
  // %24 = fsub double %22, %23
  // %25 = fsub double %0, %24
  // %26 = fsub double %1, %23
  // %27 = fadd double %26, %25
  // clang-format on
  void insertTwoSum(IRBuilder<> &Builder, Value *a, Value *b, Value **sigma,
                    Value **tau) {
    Value *sub1 = Builder.CreateFSub(*sigma, a);
    Value *sub2 = Builder.CreateFSub(*sigma, sub1);
    Value *sub3 = Builder.CreateFSub(a, sub2);
    Value *sub4 = Builder.CreateFSub(b, sub1);
    *tau = Builder.CreateFAdd(sub4, sub3, "tau");
  }

  // clang-format off
  // %22 = fmul double %0, %1
  // %23 = fneg double %22
  // %24 = call double @llvm.fma.f64(double %0, double %1, double %23) #12
  // clang-format on
  void insertTwoProduct(IRBuilder<> &Builder, Value *a, Value *b, Value **sigma,
                        Value **tau) {
    *sigma = Builder.CreateFMul(a, b, "sigma");
    Value *neg = Builder.CreateFNeg(*sigma);
    std::vector<Value *> args = {a, b, neg};
    Type *retTy = a->getType();
    *tau = CREATE_FMA_CALL(Builder, retTy, args);
  }

  // start sr_round_b64
  //   %22 = fadd double %0, %1  ; actual fadd
  //   %23 = fsub double %22, %0
  //   %24 = fsub double %22, %23
  //   %25 = fsub double %0, %24
  //   %26 = fsub double %1, %23
  //   %27 = fadd double %26, %25
  //   %28 = bitcast double %22 to i64
  //   %29 = and i64 %28, 9218868437227405312
  //   %30 = bitcast i64 %29 to double
  //   %31 = fmul double %30, 0x3CB0000000000000
  //   %32 = fmul double %31, %21
  //   %33 = fadd double %27, %32
  //   %34 = bitcast double %33 to i64
  //   %35 = and i64 %34, 9223372036854775807
  //   %36 = bitcast i64 %35 to double
  //   %37 = bitcast double %31 to i64
  //   %38 = and i64 %37, 9223372036854775807
  //   %39 = bitcast i64 %38 to double
  //   %40 = fcmp ult double %36, %39
  //   %41 = select i1 %40, double 0.000000e+00, double %31
  //   %42 = fadd double %22, %41
  //   ret double %42
  // end sr_round_b64
  Value *insertSRRounding(IRBuilder<> &Builder, Value *sigma, Value *tau,
                          Value *rand_double01) {

    Type *srcTy = sigma->getType();
    bool isVector = sigma->getType()->isVectorTy();
    VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(srcTy);
    Type *fpTy = (isVector) ? VT->getElementType() : srcTy;
    auto size = (isVector) ? VT->getNumElements() : 1;

    double ulpValue = 0.0;
    Value *ulp = nullptr;
    Constant *getExponent = nullptr, *getSign = nullptr;

    if (fpTy->isFloatTy()) {
      ulpValue = c99hextodouble("0x1.0p-23");
      getExponent = Builder.getInt32(0x7f800000);
      getSign = Builder.getInt32(0x7fffffff);
    } else {
      ulpValue = c99hextodouble("0x1.0p-52");
      getExponent = Builder.getInt64(0x7ff0000000000000);
      getSign = Builder.getInt64(0x7fffffffffffffff);
    }

    ulp = ConstantFP::get(srcTy, ulpValue);

    if (isVector) {
      auto count = CREATE_VECTOR_ELEMENT_COUNT(size);
      getExponent = ConstantVector::getSplat(count, getExponent);
      getSign = ConstantVector::getSplat(count, getSign);
    }

    Type *fpAsIntTy = getFPAsIntType(fpTy);
    if (isVector) {
      fpAsIntTy = GET_VECTOR_TYPE(fpAsIntTy, size);
    }

    // if a float,  cast rand_double01 to float
    if (fpTy->isFloatTy()) {
      rand_double01 = Builder.CreateFPCast(rand_double01, srcTy, "float cast");
    }

    // depending on the src type
    Value *bitCast = Builder.CreateBitCast(sigma, fpAsIntTy);
    Value *and1 = Builder.CreateAnd(bitCast, getExponent);
    Value *bitCast2 = Builder.CreateBitCast(and1, srcTy);
    Value *fmul = Builder.CreateFMul(bitCast2, ulp);

    Value *fmul2 = Builder.CreateFMul(fmul, rand_double01, "ulp * z");

    Value *fadd = Builder.CreateFAdd(tau, fmul2);
    Value *bitCast3 = Builder.CreateBitCast(fadd, fpAsIntTy);

    Value *and2 = Builder.CreateAnd(bitCast3, getSign);
    Value *bitCast4 = Builder.CreateBitCast(and2, srcTy);
    Value *bitCast5 = Builder.CreateBitCast(fmul, fpAsIntTy);
    Value *and3 = Builder.CreateAnd(bitCast5, getSign);
    Value *bitCast6 = Builder.CreateBitCast(and3, srcTy);
    Value *ult = Builder.CreateFCmpULT(bitCast4, bitCast6);

    Value *zero = nullptr;
    if (isVector) {
      auto count = CREATE_VECTOR_ELEMENT_COUNT(size);
      zero = ConstantVector::getSplat(count, ConstantFP::get(fpTy, 0.0));
    } else {
      zero = ConstantFP::get(fpTy, 0.0);
    }

    Value *select = Builder.CreateSelect(ult, zero, fmul);
    Value *fadd2 = Builder.CreateFAdd(sigma, select);
    return fadd2;
  }

  // ; Function Attrs: mustprogress nofree nosync nounwind uwtable willreturn
  // define dso_local double @add2_double(double noundef %0, double noundef
  // %1) local_unnamed_addr #2 { start get_rand_uint64
  //   %3 = load i64, i64* @rng_state.0, align 16, !tbaa !5
  //   %4 = load i64, i64* @rng_state.1, align 16, !tbaa !5
  //   %5 = add i64 %4, %3
  //   %6 = shl i64 %5, 17
  //   %7 = lshr i64 %5, 47
  //   %8 = add i64 %7, %3
  //   %9 = or i64 %8, %6
  //   %10 = xor i64 %4, %3
  //   %11 = shl i64 %3, 49
  //   %12 = lshr i64 %3, 15
  //   %13 = xor i64 %10, %12
  //   %14 = shl i64 %10, 21
  //   %15 = xor i64 %13, %14
  //   %16 = or i64 %15, %11
  //   store i64 %16, i64* @rng_state.0, align 16, !tbaa !5
  //   %17 = call i64 @llvm.fshl.i64(i64 %10, i64 %10, i64 28) #12
  //   store i64 %17, i64* @rng_state.1, align 16, !tbaa !5
  // end get_rand_uint64
  // start get_rand_double01
  //   %18 = lshr i64 %9, 12
  //   %19 = or i64 %18, 4607182418800017408
  //   %20 = bitcast i64 %19 to double
  //   %21 = fadd double %20, -1.000000e+00
  // end get_rand_double01
  //   %22 = fadd double %0, %1  ; actual fadd
  // start twosum
  //   %23 = fsub double %22, %0
  //   %24 = fsub double %22, %23
  //   %25 = fsub double %0, %24
  //   %26 = fsub double %1, %23
  //   %27 = fadd double %26, %25
  // end twosum
  // start sr_round_b64
  //   %28 = bitcast double %22 to i64
  //   %29 = and i64 %28, 9218868437227405312
  //   %30 = bitcast i64 %29 to double
  //   %31 = fmul double %30, 0x3CB0000000000000
  //   %32 = fmul double %31, %21
  //   %33 = fadd double %27, %32
  //   %34 = bitcast double %33 to i64
  //   %35 = and i64 %34, 9223372036854775807
  //   %36 = bitcast i64 %35 to double
  //   %37 = bitcast double %31 to i64
  //   %38 = and i64 %37, 9223372036854775807
  //   %39 = bitcast i64 %38 to double
  //   %40 = fcmp ult double %36, %39
  //   %41 = select i1 %40, double 0.000000e+00, double %31
  //   %42 = fadd double %22, %41
  //   ret double %42
  // end sr_round_b64
  // }
  Value *createSrAdd(IRBuilder<> &Builder, Instruction *I,
                     Function::arg_iterator args) {

    Value *a = static_cast<Value *>(&args[0]);
    Value *b = static_cast<Value *>(&args[1]);

    Value *rand_double01 = nullptr;
    bool isVector = a->getType()->isVectorTy();
    if (isVector) {
      rand_double01 = insertVectorizeRandDouble01Call(Builder, I);
    } else {
      rand_double01 = insertRandDouble01Call(Builder, I);
    }

    Value *tau = nullptr, *sigma = nullptr;
    sigma = Builder.CreateFAdd(a, b, "sigma");
    insertTwoSum(Builder, a, b, &sigma, &tau);
    Value *sr_round = insertSRRounding(Builder, sigma, tau, rand_double01);
    return sr_round;
  }

  Value *createSrSub(IRBuilder<> &Builder, Instruction *I,
                     Function::arg_iterator args) {
    Value *a = static_cast<Value *>(&args[0]);
    Value *b = static_cast<Value *>(&args[1]);

    bool isVector = a->getType()->isVectorTy();
    Value *rand_double01 = nullptr;
    if (isVector) {
      rand_double01 = insertVectorizeRandDouble01Call(Builder, I);
    } else {
      rand_double01 = insertRandDouble01Call(Builder, I);
    }

    Value *tau = nullptr, *sigma = nullptr;
    sigma = Builder.CreateFSub(a, b, "sigma");
    b = Builder.CreateFNeg(b);
    insertTwoSum(Builder, a, b, &sigma, &tau);
    Value *sr_round = insertSRRounding(Builder, sigma, tau, rand_double01);
    return sr_round;
  }

  // clang-format off
  //  Function Attrs: mustprogress nofree nosync nounwind uwtable willreturn
  // define dso_local double @mul2_double(double noundef %0, double noundef %1) local_unnamed_addr #2 {
  // start get_rand_uint64
  //   %3 = load i64, i64* @rng_state.0, align 16, !tbaa !5
  //   %4 = load i64, i64* @rng_state.1, align 16, !tbaa !5
  //   %5 = add i64 %4, %3
  //   %6 = shl i64 %5, 17
  //   %7 = lshr i64 %5, 47
  //   %8 = add i64 %7, %3
  //   %9 = or i64 %8, %6
  //   %10 = xor i64 %4, %3
  //   %11 = shl i64 %3, 49
  //   %12 = lshr i64 %3, 15
  //   %13 = xor i64 %10, %12
  //   %14 = shl i64 %10, 21
  //   %15 = xor i64 %13, %14
  //   %16 = or i64 %15, %11
  //   store i64 %16, i64* @rng_state.0, align 16, !tbaa !5
  //   %17 = call i64 @llvm.fshl.i64(i64 %10, i64 %10, i64 28) #12
  //   store i64 %17, i64* @rng_state.1, align 16, !tbaa !5
  // end get_rand_uint64
  // start get_rand_double01
  //   %18 = lshr i64 %9, 12
  //   %19 = or i64 %18, 4607182418800017408
  //   %20 = bitcast i64 %19 to double
  //   %21 = fadd double %20, -1.000000e+00
  // end get_rand_double01
  // start twoproduct
  //   %22 = fmul double %0, %1
  //   %23 = fneg double %22
  //   %24 = call double @llvm.fma.f64(double %0, double %1, double %23) #12
  // end twoproduct
  // start sr_round_b64
  //   %25 = bitcast double %22 to i64
  //   %26 = and i64 %25, 9218868437227405312
  //   %27 = bitcast i64 %26 to double
  //   %28 = fmul double %27, 0x3CB0000000000000
  //   %29 = fmul double %28, %21
  //   %30 = fadd double %24, %29
  //   %31 = bitcast double %30 to i64
  //   %32 = and i64 %31, 9223372036854775807
  //   %33 = bitcast i64 %32 to double
  //   %34 = bitcast double %28 to i64
  //   %35 = and i64 %34, 9223372036854775807
  //   %36 = bitcast i64 %35 to double
  //   %37 = fcmp ult double %33, %36
  //   %38 = select i1 %37, double 0.000000e+00, double %28
  //   %39 = fadd double %22, %38
  //   ret double %39
  // end sr_round_b64
  // }
  // }
  // clang-format on
  Value *createSrMul(IRBuilder<> &Builder, Instruction *I,
                     Function::arg_iterator args) {
    Value *a = static_cast<Value *>(&args[0]);
    Value *b = static_cast<Value *>(&args[1]);

    Value *sigma = nullptr, *tau = nullptr;

    bool isVector = a->getType()->isVectorTy();
    Value *rand_double01 = nullptr;
    if (isVector) {
      rand_double01 = insertVectorizeRandDouble01Call(Builder, I);
    } else {
      rand_double01 = insertRandDouble01Call(Builder, I);
    }

    insertTwoProduct(Builder, a, b, &sigma, &tau);
    Value *sr_round = insertSRRounding(Builder, sigma, tau, rand_double01);
    return sr_round;
  }

  Value *createSrDiv(IRBuilder<> &Builder, Instruction *I,
                     Function::arg_iterator args) {
    Value *a = static_cast<Value *>(&args[0]);
    Value *b = static_cast<Value *>(&args[1]);

    Value *sigma = nullptr, *tau = nullptr;

    Value *rand_double01 = nullptr;
    bool isVector = a->getType()->isVectorTy();
    if (isVector) {
      rand_double01 = insertVectorizeRandDouble01Call(Builder, I);
    } else {
      rand_double01 = insertRandDouble01Call(Builder, I);
    }

    // clang-format off
    // %22 = fdiv double %0, %1
    // %23 = fneg double %22
    // %24 = call double @llvm.fma.f64(double %23, double %1, double %0)
    // %25 = fdiv double %24, %1
    // clang-format on
    sigma = Builder.CreateFDiv(a, b);
    Value *neg = Builder.CreateFNeg(sigma);
    std::vector<Value *> args_fma = {neg, b, a};
    Type *fmaRetType = a->getType();
    Value *fma = CREATE_FMA_CALL(Builder, fmaRetType, args_fma);
    tau = Builder.CreateFDiv(fma, b);
    Value *sr_round = insertSRRounding(Builder, sigma, tau, rand_double01);
    return sr_round;
  }

  Value *createSrFMA(IRBuilder<> &Builder, Instruction *I,
                     Function::arg_iterator args) {
    Value *a = static_cast<Value *>(&args[0]);
    Value *b = static_cast<Value *>(&args[1]);
    Value *c = static_cast<Value *>(&args[2]);

    Value *ph = nullptr, *pl = nullptr, *uh = nullptr, *ul = nullptr;

    bool isVector = a->getType()->isVectorTy();
    Value *rand_double01 = nullptr;
    if (isVector) {
      rand_double01 = insertVectorizeRandDouble01Call(Builder, I);
    } else {
      rand_double01 = insertRandDouble01Call(Builder, I);
    }

    insertTwoProduct(Builder, a, b, &ph, &pl);
    uh = Builder.CreateFAdd(c, ph);
    insertTwoSum(Builder, c, ph, &uh, &ul);

    std::vector<Value *> args_sigma = {a, b, c};
    Type *retTy = a->getType();
    Value *sigma = CREATE_FMA_CALL(Builder, retTy, args_sigma);
    Value *t = Builder.CreateFSub(uh, sigma);
    Value *error = Builder.CreateFAdd(pl, ul);
    Value *tau = Builder.CreateFAdd(error, t);
    Value *sr_round = insertSRRounding(Builder, sigma, tau, rand_double01);
    return sr_round;
  }

  std::string getFunctionName(Fops opCode) {
    switch (opCode) {
    case Fops::FOP_ADD:
      return "add2";
    case Fops::FOP_SUB:
      return "sub2";
    case Fops::FOP_MUL:
      return "mul2";
    case Fops::FOP_DIV:
      return "div2";
    case Fops::FOP_FMA:
      return "fma2";
    default:
      errs() << "Unsupported opcode: " << opCode << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Value *createSrOp(IRBuilder<> &Builder, Instruction *I,
                    Function::arg_iterator args, Fops opCode) {
    switch (opCode) {
    case Fops::FOP_ADD:
      return createSrAdd(Builder, I, args);
    case Fops::FOP_SUB:
      return createSrSub(Builder, I, args);
    case Fops::FOP_MUL:
      return createSrMul(Builder, I, args);
    case Fops::FOP_DIV:
      return createSrDiv(Builder, I, args);
    case Fops::FOP_FMA:
      return createSrFMA(Builder, I, args);
    default:
      errs() << "Unsupported opcode: " << opCode << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Value *getOrCreateSrFunction(IRBuilder<> &Builder, Instruction *I,
                               Fops opCode) {
    Module *M = Builder.GetInsertBlock()->getParent()->getParent();
    Type *srcTy = I->getType();

    std::string function_name =
        getFunctionName(opCode) + "_" + getTypeName(srcTy);

    // if instruction is vector, update the name
    if (srcTy->isVectorTy()) {
      VectorType *VT = dyn_cast<VectorType>(srcTy);
#if LLVM_VERSION_MAJOR < 9
      unsigned int size = VT->getNumElements();
#else
      ElementCount EC = VT->getElementCount();
      unsigned int size = EC.getFixedValue();
#endif
      function_name += "v" + std::to_string(size);
    }

    Function *function = I->getModule()->getFunction(function_name);

    if (function == nullptr or function->empty()) {
      Type *voidTy = Type::getVoidTy(I->getContext());

      FunctionType *funcType = nullptr;
      if (opCode == Fops::FOP_FMA) {
        funcType = FunctionType::get(srcTy, {srcTy, srcTy, srcTy}, false);
      } else {
        funcType = FunctionType::get(srcTy, {srcTy, srcTy}, false);
      }
      if (function == nullptr) {
        function = Function::Create(funcType, Function::InternalLinkage,
                                    function_name, M);
      }

      BasicBlock *BB = BasicBlock::Create(I->getContext(), "entry", function);
      Builder.SetInsertPoint(BB);

      Function::arg_iterator args = function->arg_begin();
      Value *sr_op = createSrOp(Builder, I, &*args, opCode);
      Builder.CreateRet(sr_op);
      Builder.ClearInsertionPoint();
    }

    // Check if this instruction is the last one in the block
    if (I->isTerminator()) {
      // Insert at the end of the block
      Builder.SetInsertPoint(I->getParent());
    } else {
      // Set insertion point after the given instruction
      Builder.SetInsertPoint(I->getNextNode());
    }

    if (opCode == Fops::FOP_FMA) {
      std::vector<Value *> args = {I->getOperand(0), I->getOperand(1),
                                   I->getOperand(2)};
      return Builder.CreateCall(function, args);
    } else {
      std::vector<Value *> args = {I->getOperand(0), I->getOperand(1)};
      return Builder.CreateCall(function, args);
    }
  }

  Value *replaceArithmeticWithMCACall_SR(IRBuilder<> &Builder, Instruction *I,
                                         std::set<User *> &users) {
    Fops opCode = mustReplace(*I);
    Value *value = getOrCreateSrFunction(Builder, I, opCode);
    return value;
  }

  Value *replaceArithmeticWithMCACall_UpOrDown(IRBuilder<> &Builder,
                                               Instruction *I,
                                               std::set<User *> &users) {
    Type *srcTy = nullptr;
    bool isVector = I->getType()->isVectorTy();
    VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
    srcTy = (isVector) ? VT->getElementType() : I->getType();

    // add check to compare the result with 0 and return 0 if the result is 0
    Constant *zeroFP = ConstantFP::get(I->getType(), 0.0);

    Value *cmp = Builder.CreateFCmpOEQ(I, zeroFP, "is_zero");
    users.insert(static_cast<User *>(cmp));

    Type *fpAsIntTy = getFPAsIntType(srcTy);
    Value *randomBits = nullptr;
    if (isVector) {
      randomBits = insertVectorizeRandUint64Call(Builder, I);
    } else {
      randomBits = insertRandUint64Call(Builder, I);
    }

    if (srcTy->isFloatTy()) {
      Type *int32Ty = Type::getInt32Ty(Builder.getContext());
      Type *vecTy = nullptr;
      if (isVector) {
        VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
        vecTy = VECTOR_TYPE::get(int32Ty, VT->getNumElements());
      } else {
        vecTy = int32Ty;
      }
      randomBits = Builder.CreateTrunc(randomBits, vecTy);
    }

    Constant *one = ConstantInt::get(fpAsIntTy, 1);
    Constant *mone = ConstantInt::get(fpAsIntTy, -1);

    if (isVector) {
      VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
      one = ConstantVector::getSplat(VT->getElementCount(), one);
      mone = ConstantVector::getSplat(VT->getElementCount(), mone);
    }

    Value *randomBit = Builder.CreateAnd(randomBits, one, "get_random_bit");

    Type *boolTy = Type::getInt1Ty(Builder.getContext());
    Type *truncTy = nullptr;
    if (isVector) {
      VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
      truncTy = VECTOR_TYPE::get(boolTy, VT->getNumElements());
    } else {
      truncTy = boolTy;
    }

    randomBit = Builder.CreateTrunc(randomBit, truncTy, "random_bit_to_bool");
    // Add +1 or -1 to fpAsInt depending on the random bit
    Value *noise = Builder.CreateSelect(randomBit, one, mone, "noise");

    Type *bitCastTy = nullptr;
    if (isVector) {
      VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
      bitCastTy = VECTOR_TYPE::get(fpAsIntTy, VT->getNumElements());
    } else {
      bitCastTy = fpAsIntTy;
    }

    Value *FPAsInt = Builder.CreateBitCast(I, bitCastTy, "fpAsInt");
    users.insert(static_cast<User *>(FPAsInt));
    Value *newResult = Builder.CreateAdd(FPAsInt, noise, "add_noise");
    Value *fpNoised =
        Builder.CreateBitCast(newResult, I->getType(), "fp_noised");

    Instruction *select = static_cast<Instruction *>(
        Builder.CreateSelect(cmp, zeroFP, fpNoised, "select 0 if zero"));

    return select;
  }

  /* Replace arithmetic instructions with MCA */
  Value *replaceArithmeticWithMCACall(IRBuilder<> &Builder, Instruction *I,
                                      std::set<User *> &users) {
    if (VfclibInstMode == "up-down") {
      return replaceArithmeticWithMCACall_UpOrDown(Builder, I, users);
    } else if (VfclibInstMode == "sr") {
      return replaceArithmeticWithMCACall_SR(Builder, I, users);
    } else {
      errs() << "Unsupported mode: " << VfclibInstMode << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Value *replaceWithMCACall(Module &M, Instruction *I, Fops opCode,
                            std::set<User *> &users) {
    if (not isValidInstruction(I)) {
      return nullptr;
    }

    IRBuilder<> Builder(I->getContext());
    // Check if this instruction is the last one in the block
    if (I->isTerminator()) {
      // Insert at the end of the block
      Builder.SetInsertPoint(I->getParent());
    } else {
      // Set insertion point after the given instruction
      Builder.SetInsertPoint(I->getNextNode());
    }

    // We call directly a hardcoded helper function
    // no need to go through the vtable at this stage.
    Value *newInst = replaceArithmeticWithMCACall(Builder, I, users);
    return newInst;
  }

  bool isFMAOperation(Instruction &I) {
    CallInst *CI = static_cast<CallInst *>(&I);
    if (CI->getCalledFunction() == nullptr)
      return false;
    const std::string &name = CI->getCalledFunction()->getName().str();
    if (name == "llvm.fmuladd.f32")
      return true;
    if (name == "llvm.fmuladd.f64")
      return true;
    if (name == "llvm.fma.f32")
      return true;
    if (name == "llvm.fma.f64")
      return true;
    return false;
  }

  Fops mustReplace(Instruction &I) {
    switch (I.getOpcode()) {
    case Instruction::FAdd:
      return FOP_ADD;
    case Instruction::FSub:
      // In LLVM IR the FSub instruction is used to represent FNeg
      return FOP_SUB;
    case Instruction::FMul:
      return FOP_MUL;
    case Instruction::FDiv:
      return FOP_DIV;
    case Instruction::Call:
      // Only instrument FMA if the flag --inst-fma is passed
      if (VfclibInstInstrumentFMA and isFMAOperation(I)) {
        return FOP_FMA;
      } else {
        return FOP_IGNORE;
      }
    case Instruction::FCmp:
      return FOP_IGNORE;
    default:
      return FOP_IGNORE;
    }
  }

  void replaceUsageWith(std::set<User *> &users, Value *from, Value *to) {
    for (User *user : from->users()) {
      if (Instruction *ii = dyn_cast<Instruction>(user)) {
        if (users.find(ii) == users.end()) {
          for (auto &op : ii->operands()) {
            if (op.get() == from) {
              op.set(to);
            }
          }
        }
      }
    }
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {
    bool modified = false;
    std::set<std::pair<Instruction *, Fops>> WorkList;
    for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
      Instruction &I = *ii;
      Fops opCode = mustReplace(I);
      if (opCode == FOP_IGNORE)
        continue;
      WorkList.insert(std::make_pair(&I, opCode));
    }

    for (auto p : WorkList) {
      Instruction *I = p.first;
      Fops opCode = p.second;
      if (VfclibInstVerbose)
        errs() << "Instrumenting" << *I << '\n';
      std::set<User *> fp_users;
      Value *value = replaceWithMCACall(M, I, opCode, fp_users);
      if (value != nullptr and VfclibInstMode != "up-down") {
        I->replaceAllUsesWith(value);
        I->eraseFromParent();
      } else if (value != nullptr and VfclibInstMode == "up-down") {
        // We need to replace the original instruction with the noised
        // instruction for all the users of the original instruction after the
        // noise is added
        replaceUsageWith(fp_users, I, value);
      }
      modified = true;
    }

    return modified;
  }
}; // namespace
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass",
                                  false, false);
