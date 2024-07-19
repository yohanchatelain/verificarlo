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
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/SourceMgr.h>
#if LLVM_VERSION_MAJOR >= 11
#undef PIC
#include <llvm/MC/TargetRegistry.h>
#else
#include <llvm/Support/TargetRegistry.h>
#endif
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#pragma GCC diagnostic pop

#include <cmath>
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

static cl::opt<std::string> VfclibInstRNG("vfclibinst-rng",
                                          cl::desc("RNG function name"),
                                          cl::value_desc("RNG"),
                                          cl::init("xoroshiro"));

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

/* SHISHUA buffer size */
const unsigned shishua_buffer_size = 256;

struct VfclibInst : public ModulePass {
  static char ID;

  VfclibInst() : ModulePass(ID) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
  }

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

  void insertRandUint64XoroshiroCall(IRBuilder<> &Builder, Instruction *I,
                                     Value **rand) {
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
    *rand = Builder.CreateOr(xor4, shl2, "rand_uint64");
    Builder.CreateStore(*rand, rng_state0);
#if LLVM_VERSION_MAJOR < 9
    std::vector<Value *> args = {xor2, xor2, Builder.getInt64(28)};
    Value *fshl = Builder.CreateIntrinsic(Intrinsic::fshl, args);
#else
    Value *fshl = Builder.CreateIntrinsic(Intrinsic::fshl, {xor2->getType()},
                                          {xor2, xor2, Builder.getInt64(28)});
#endif
    Builder.CreateStore(fshl, rng_state1);
  }

  template <typename T>
  Value *getVectorConstant(Module *M, std::initializer_list<T> values) {
    // define Type *ty depending on the type of T
    Type *ty = nullptr;
    if (std::is_same<T, int32_t>::value or std::is_same<T, uint32_t>::value) {
      ty = Type::getInt32Ty(M->getContext());
    } else if (std::is_same<T, int64_t>::value or
               std::is_same<T, uint64_t>::value) {
      ty = Type::getInt64Ty(M->getContext());
    } else {
      errs() << "Unsupported type: " << typeid(T).name() << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }

    std::vector<Constant *> constants;
    for (auto value : values) {
      constants.push_back(ConstantInt::get(ty, value));
    }
    return ConstantVector::get(constants);
  }

  template <typename T>
  Value *getVectorConstant(Module *M, std::vector<T> values) {
    // define Type *ty depending on the type of T
    Type *ty = nullptr;
    if (std::is_same<T, int32_t>::value or std::is_same<T, uint32_t>::value) {
      ty = Type::getInt32Ty(M->getContext());
    } else if (std::is_same<T, int64_t>::value or
               std::is_same<T, uint64_t>::value) {
      ty = Type::getInt64Ty(M->getContext());
    } else {
      errs() << "Unsupported type: " << typeid(T).name() << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }

    std::vector<Constant *> constants;
    for (auto value : values) {
      constants.push_back(ConstantInt::get(ty, value));
    }
    return ConstantVector::get(constants);
  }

  // clang-format off
// ; Function Attrs: nofree nosync nounwind uwtable
// define dso_local void @prng_init(%struct.prng_state* nocapture noundef %0, i64* nocapture noundef readonly %1) local_unnamed_addr #0 {
//   %3 = bitcast %struct.prng_state* %0 to i8*
//   tail call void @llvm.memset.p0i8.i64(i8* noundef nonnull align 32 dereferenceable(288) %3, i8 0, i64 288, i1 false)
//   %4 = bitcast i64* %1 to <2 x i64>*
//   %5 = load <2 x i64>, <2 x i64>* %4, align 8, !tbaa !5
//   %6 = xor <2 x i64> %5, <i64 -7046029254386353131, i64 1189556596181725777>
//   %7 = shufflevector <2 x i64> %6, <2 x i64> poison, <4 x i32> <i32 0, i32 undef, i32 1, i32 undef>
//   %8 = shufflevector <4 x i64> %7, <4 x i64> <i64 poison, i64 -892627106017720268, i64 poison, i64 -545944830069338475>, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
//   %9 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 0, i64 0
//   store <4 x i64> %8, <4 x i64>* %9, align 32, !tbaa !9
//   %10 = getelementptr inbounds i64, i64* %1, i64 2
//   %11 = bitcast i64* %10 to <2 x i64>*
//   %12 = load <2 x i64>, <2 x i64>* %11, align 8, !tbaa !5
//   %13 = xor <2 x i64> %12, <i64 2839502734486567807, i64 110460275991261186>
//   %14 = shufflevector <2 x i64> %13, <2 x i64> poison, <4 x i32> <i32 0, i32 undef, i32 1, i32 undef>
//   %15 = shufflevector <4 x i64> %14, <4 x i64> <i64 poison, i64 236162295891329663, i64 poison, i64 -4479309611371635146>, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
//   %16 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 0, i64 1
//   store <4 x i64> %15, <4 x i64>* %16, align 32, !tbaa !9
//   %17 = bitcast i64* %10 to <2 x i64>*
//   %18 = load <2 x i64>, <2 x i64>* %17, align 8, !tbaa !5
//   %19 = xor <2 x i64> %18, <i64 -1122848012216334466, i64 7265192206765856386>
//   %20 = shufflevector <2 x i64> %19, <2 x i64> poison, <4 x i32> <i32 0, i32 undef, i32 1, i32 undef>
//   %21 = shufflevector <4 x i64> %20, <4 x i64> <i64 poison, i64 -8826037744653468218, i64 poison, i64 -3828890033620796447>, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
//   %22 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 0, i64 2
//   store <4 x i64> %21, <4 x i64>* %22, align 32, !tbaa !9
//   %23 = bitcast i64* %1 to <2 x i64>*
//   %24 = load <2 x i64>, <2 x i64>* %23, align 8, !tbaa !5
//   %25 = xor <2 x i64> %24, <i64 7092663332016702257, i64 5124052612680614565>
//   %26 = shufflevector <2 x i64> %25, <2 x i64> poison, <4 x i32> <i32 0, i32 undef, i32 1, i32 undef>
//   %27 = shufflevector <4 x i64> %26, <4 x i64> <i64 poison, i64 -4902383178752919651, i64 poison, i64 -88656438464157979>, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
//   br label %35

// 28:                                               ; preds = %35
//   %29 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 0, i64 3
//   %30 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 2
//   %31 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 1, i64 3
//   %32 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 1, i64 2
//   %33 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 1, i64 1
//   %34 = getelementptr inbounds %struct.prng_state, %struct.prng_state* %0, i64 0, i32 1, i64 0
//   store <4 x i64> %228, <4 x i64>* %9, align 32, !tbaa !9
//   store <4 x i64> %227, <4 x i64>* %16, align 32, !tbaa !9
//   store <4 x i64> %226, <4 x i64>* %22, align 32, !tbaa !9
//   store <4 x i64> %225, <4 x i64>* %29, align 32, !tbaa !9
//   store <4 x i64> %229, <4 x i64>* %30, align 32, !tbaa !9
//   store <4 x i64> %225, <4 x i64>* %34, align 32, !tbaa !9
//   store <4 x i64> %226, <4 x i64>* %33, align 32, !tbaa !9
//   store <4 x i64> %227, <4 x i64>* %32, align 32, !tbaa !9
//   store <4 x i64> %228, <4 x i64>* %31, align 32, !tbaa !9
//   ret void

// 35:                                               ; preds = %2, %35
//   %36 = phi i64 [ 0, %2 ], [ %230, %35 ]
//   %37 = phi <4 x i64> [ %8, %2 ], [ %228, %35 ]
//   %38 = phi <4 x i64> [ %15, %2 ], [ %227, %35 ]
//   %39 = phi <4 x i64> [ %21, %2 ], [ %226, %35 ]
//   %40 = phi <4 x i64> [ %27, %2 ], [ %225, %35 ]
//   %41 = phi <4 x i64> [ zeroinitializer, %2 ], [ %229, %35 ]
//   %42 = add <4 x i64> %41, %38
//   %43 = add <4 x i64> %41, %40
//   %44 = or <4 x i64> %41, <i64 7, i64 5, i64 3, i64 1>
//   %45 = lshr <4 x i64> %37, <i64 1, i64 1, i64 1, i64 1>
//   %46 = lshr <4 x i64> %42, <i64 3, i64 3, i64 3, i64 3>
//   %47 = lshr <4 x i64> %39, <i64 1, i64 1, i64 1, i64 1>
//   %48 = lshr <4 x i64> %43, <i64 3, i64 3, i64 3, i64 3>
//   %49 = bitcast <4 x i64> %37 to <8 x i32>
//   %50 = shufflevector <8 x i32> %49, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %51 = bitcast <8 x i32> %50 to <4 x i64>
//   %52 = bitcast <4 x i64> %42 to <8 x i32>
//   %53 = shufflevector <8 x i32> %52, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %54 = bitcast <8 x i32> %53 to <4 x i64>
//   %55 = bitcast <4 x i64> %39 to <8 x i32>
//   %56 = shufflevector <8 x i32> %55, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %57 = bitcast <8 x i32> %56 to <4 x i64>
//   %58 = bitcast <4 x i64> %43 to <8 x i32>
//   %59 = shufflevector <8 x i32> %58, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %60 = bitcast <8 x i32> %59 to <4 x i64>
//   %61 = add <4 x i64> %45, %51
//   %62 = add <4 x i64> %47, %57
//   %63 = add <4 x i64> %46, %44
//   %64 = add <4 x i64> %63, %54
//   %65 = add <4 x i64> %48, %44
//   %66 = add <4 x i64> %65, %60
//   %67 = add <4 x i64> %41, <i64 14, i64 10, i64 6, i64 2>
//   %68 = lshr <4 x i64> %61, <i64 1, i64 1, i64 1, i64 1>
//   %69 = lshr <4 x i64> %64, <i64 3, i64 3, i64 3, i64 3>
//   %70 = lshr <4 x i64> %62, <i64 1, i64 1, i64 1, i64 1>
//   %71 = lshr <4 x i64> %66, <i64 3, i64 3, i64 3, i64 3>
//   %72 = bitcast <4 x i64> %61 to <8 x i32>
//   %73 = shufflevector <8 x i32> %72, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %74 = bitcast <8 x i32> %73 to <4 x i64>
//   %75 = bitcast <4 x i64> %64 to <8 x i32>
//   %76 = shufflevector <8 x i32> %75, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %77 = bitcast <8 x i32> %76 to <4 x i64>
//   %78 = bitcast <4 x i64> %62 to <8 x i32>
//   %79 = shufflevector <8 x i32> %78, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %80 = bitcast <8 x i32> %79 to <4 x i64>
//   %81 = bitcast <4 x i64> %66 to <8 x i32>
//   %82 = shufflevector <8 x i32> %81, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %83 = bitcast <8 x i32> %82 to <4 x i64>
//   %84 = add <4 x i64> %68, %74
//   %85 = add <4 x i64> %70, %80
//   %86 = add <4 x i64> %69, %67
//   %87 = add <4 x i64> %86, %77
//   %88 = add <4 x i64> %71, %67
//   %89 = add <4 x i64> %88, %83
//   %90 = add <4 x i64> %41, <i64 21, i64 15, i64 9, i64 3>
//   %91 = lshr <4 x i64> %84, <i64 1, i64 1, i64 1, i64 1>
//   %92 = lshr <4 x i64> %87, <i64 3, i64 3, i64 3, i64 3>
//   %93 = lshr <4 x i64> %85, <i64 1, i64 1, i64 1, i64 1>
//   %94 = lshr <4 x i64> %89, <i64 3, i64 3, i64 3, i64 3>
//   %95 = bitcast <4 x i64> %84 to <8 x i32>
//   %96 = shufflevector <8 x i32> %95, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %97 = bitcast <8 x i32> %96 to <4 x i64>
//   %98 = bitcast <4 x i64> %87 to <8 x i32>
//   %99 = shufflevector <8 x i32> %98, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %100 = bitcast <8 x i32> %99 to <4 x i64>
//   %101 = bitcast <4 x i64> %85 to <8 x i32>
//   %102 = shufflevector <8 x i32> %101, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %103 = bitcast <8 x i32> %102 to <4 x i64>
//   %104 = bitcast <4 x i64> %89 to <8 x i32>
//   %105 = shufflevector <8 x i32> %104, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %106 = bitcast <8 x i32> %105 to <4 x i64>
//   %107 = add <4 x i64> %91, %97
//   %108 = add <4 x i64> %93, %103
//   %109 = add <4 x i64> %92, %90
//   %110 = add <4 x i64> %109, %100
//   %111 = add <4 x i64> %94, %90
//   %112 = add <4 x i64> %111, %106
//   %113 = add <4 x i64> %41, <i64 28, i64 20, i64 12, i64 4>
//   %114 = lshr <4 x i64> %107, <i64 1, i64 1, i64 1, i64 1>
//   %115 = lshr <4 x i64> %110, <i64 3, i64 3, i64 3, i64 3>
//   %116 = lshr <4 x i64> %108, <i64 1, i64 1, i64 1, i64 1>
//   %117 = lshr <4 x i64> %112, <i64 3, i64 3, i64 3, i64 3>
//   %118 = bitcast <4 x i64> %107 to <8 x i32>
//   %119 = shufflevector <8 x i32> %118, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %120 = bitcast <8 x i32> %119 to <4 x i64>
//   %121 = bitcast <4 x i64> %110 to <8 x i32>
//   %122 = shufflevector <8 x i32> %121, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %123 = bitcast <8 x i32> %122 to <4 x i64>
//   %124 = bitcast <4 x i64> %108 to <8 x i32>
//   %125 = shufflevector <8 x i32> %124, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %126 = bitcast <8 x i32> %125 to <4 x i64>
//   %127 = bitcast <4 x i64> %112 to <8 x i32>
//   %128 = shufflevector <8 x i32> %127, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %129 = bitcast <8 x i32> %128 to <4 x i64>
//   %130 = add <4 x i64> %114, %120
//   %131 = add <4 x i64> %116, %126
//   %132 = add <4 x i64> %115, %113
//   %133 = add <4 x i64> %132, %123
//   %134 = add <4 x i64> %117, %113
//   %135 = add <4 x i64> %134, %129
//   %136 = add <4 x i64> %41, <i64 35, i64 25, i64 15, i64 5>
//   %137 = lshr <4 x i64> %130, <i64 1, i64 1, i64 1, i64 1>
//   %138 = lshr <4 x i64> %133, <i64 3, i64 3, i64 3, i64 3>
//   %139 = lshr <4 x i64> %131, <i64 1, i64 1, i64 1, i64 1>
//   %140 = lshr <4 x i64> %135, <i64 3, i64 3, i64 3, i64 3>
//   %141 = bitcast <4 x i64> %130 to <8 x i32>
//   %142 = shufflevector <8 x i32> %141, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %143 = bitcast <8 x i32> %142 to <4 x i64>
//   %144 = bitcast <4 x i64> %133 to <8 x i32>
//   %145 = shufflevector <8 x i32> %144, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %146 = bitcast <8 x i32> %145 to <4 x i64>
//   %147 = bitcast <4 x i64> %131 to <8 x i32>
//   %148 = shufflevector <8 x i32> %147, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %149 = bitcast <8 x i32> %148 to <4 x i64>
//   %150 = bitcast <4 x i64> %135 to <8 x i32>
//   %151 = shufflevector <8 x i32> %150, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %152 = bitcast <8 x i32> %151 to <4 x i64>
//   %153 = add <4 x i64> %137, %143
//   %154 = add <4 x i64> %139, %149
//   %155 = add <4 x i64> %138, %136
//   %156 = add <4 x i64> %155, %146
//   %157 = add <4 x i64> %140, %136
//   %158 = add <4 x i64> %157, %152
//   %159 = add <4 x i64> %41, <i64 42, i64 30, i64 18, i64 6>
//   %160 = lshr <4 x i64> %153, <i64 1, i64 1, i64 1, i64 1>
//   %161 = lshr <4 x i64> %156, <i64 3, i64 3, i64 3, i64 3>
//   %162 = lshr <4 x i64> %154, <i64 1, i64 1, i64 1, i64 1>
//   %163 = lshr <4 x i64> %158, <i64 3, i64 3, i64 3, i64 3>
//   %164 = bitcast <4 x i64> %153 to <8 x i32>
//   %165 = shufflevector <8 x i32> %164, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %166 = bitcast <8 x i32> %165 to <4 x i64>
//   %167 = bitcast <4 x i64> %156 to <8 x i32>
//   %168 = shufflevector <8 x i32> %167, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %169 = bitcast <8 x i32> %168 to <4 x i64>
//   %170 = bitcast <4 x i64> %154 to <8 x i32>
//   %171 = shufflevector <8 x i32> %170, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %172 = bitcast <8 x i32> %171 to <4 x i64>
//   %173 = bitcast <4 x i64> %158 to <8 x i32>
//   %174 = shufflevector <8 x i32> %173, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %175 = bitcast <8 x i32> %174 to <4 x i64>
//   %176 = add <4 x i64> %160, %166
//   %177 = add <4 x i64> %162, %172
//   %178 = add <4 x i64> %161, %159
//   %179 = add <4 x i64> %178, %169
//   %180 = add <4 x i64> %163, %159
//   %181 = add <4 x i64> %180, %175
//   %182 = add <4 x i64> %41, <i64 49, i64 35, i64 21, i64 7>
//   %183 = lshr <4 x i64> %176, <i64 1, i64 1, i64 1, i64 1>
//   %184 = lshr <4 x i64> %179, <i64 3, i64 3, i64 3, i64 3>
//   %185 = lshr <4 x i64> %177, <i64 1, i64 1, i64 1, i64 1>
//   %186 = lshr <4 x i64> %181, <i64 3, i64 3, i64 3, i64 3>
//   %187 = bitcast <4 x i64> %176 to <8 x i32>
//   %188 = shufflevector <8 x i32> %187, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %189 = bitcast <8 x i32> %188 to <4 x i64>
//   %190 = bitcast <4 x i64> %179 to <8 x i32>
//   %191 = shufflevector <8 x i32> %190, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %192 = bitcast <8 x i32> %191 to <4 x i64>
//   %193 = bitcast <4 x i64> %177 to <8 x i32>
//   %194 = shufflevector <8 x i32> %193, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %195 = bitcast <8 x i32> %194 to <4 x i64>
//   %196 = bitcast <4 x i64> %181 to <8 x i32>
//   %197 = shufflevector <8 x i32> %196, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %198 = bitcast <8 x i32> %197 to <4 x i64>
//   %199 = add <4 x i64> %183, %189
//   %200 = add <4 x i64> %185, %195
//   %201 = add <4 x i64> %184, %182
//   %202 = add <4 x i64> %201, %192
//   %203 = add <4 x i64> %186, %182
//   %204 = add <4 x i64> %203, %198
//   %205 = lshr <4 x i64> %199, <i64 1, i64 1, i64 1, i64 1>
//   %206 = lshr <4 x i64> %202, <i64 3, i64 3, i64 3, i64 3>
//   %207 = lshr <4 x i64> %200, <i64 1, i64 1, i64 1, i64 1>
//   %208 = lshr <4 x i64> %204, <i64 3, i64 3, i64 3, i64 3>
//   %209 = bitcast <4 x i64> %199 to <8 x i32>
//   %210 = shufflevector <8 x i32> %209, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %211 = bitcast <8 x i32> %210 to <4 x i64>
//   %212 = bitcast <4 x i64> %202 to <8 x i32>
//   %213 = shufflevector <8 x i32> %212, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %214 = bitcast <8 x i32> %213 to <4 x i64>
//   %215 = bitcast <4 x i64> %200 to <8 x i32>
//   %216 = shufflevector <8 x i32> %215, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
//   %217 = bitcast <8 x i32> %216 to <4 x i64>
//   %218 = bitcast <4 x i64> %204 to <8 x i32>
//   %219 = shufflevector <8 x i32> %218, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
//   %220 = bitcast <8 x i32> %219 to <4 x i64>
//   %221 = add <4 x i64> %205, %211
//   %222 = add <4 x i64> %206, %214
//   %223 = add <4 x i64> %207, %217
//   %224 = add <4 x i64> %208, %220
//   %225 = xor <4 x i64> %205, %214
//   %226 = xor <4 x i64> %207, %220
//   %227 = xor <4 x i64> %224, %221
//   %228 = xor <4 x i64> %222, %223
//   %229 = add <4 x i64> %41, <i64 56, i64 40, i64 24, i64 8>
//   %230 = add nuw nsw i64 %36, 1
//   %231 = icmp eq i64 %230, 13
//   br i1 %231, label %28, label %35, !llvm.loop !10
// }
  // clang-format on

  Value *insertShishuaInit(IRBuilder<> &Builder, Instruction *I) {
    Module *M = I->getModule();
    const std::string function_name = "_shishua_init";
    if (Function *shishua = M->getFunction(function_name)) {
      return Builder.CreateCall(shishua);
    }

    auto voidTy = Type::getVoidTy(M->getContext());
    std::vector<Type *> argsTy;
    argsTy.push_back(Type::getInt32Ty(M->getContext()));
    FunctionType *shishuaInitType = FunctionType::get(voidTy, argsTy, false);

    Function *shishuaInit =
        getOrInsertFunction(M, function_name, shishuaInitType);
  }

  // clang-format off
  // ; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind uwtable willreturn
  // define dso_local i64 @get_rand_uint64() local_unnamed_addr #4 {
  // entry:
  //   %1 = load i32, i32* @get_rand_uint64.i, align 4, !tbaa !10
  //   %2 = and i32 %1, 3
  //   %3 = icmp eq i32 %2, 0
  //   br i1 %3, label %4, label %22
  // rng:                                                ; preds = %0
  //   %5 = load <4 x i64>, <4 x i64>* @rng_state.0, align 32, !tbaa !9
  //   %6 = load <4 x i64>, <4 x i64>* @rng_state.3, align 32, !tbaa !9
  //   %7 = load <4 x i64>, <4 x i64>* @rng_state.1, align 32, !tbaa !9
  //   %8 = load <4 x i64>, <4 x i64>* @rng_state.2, align 32, !tbaa !9
  //   store <4 x i64> %8, <4 x i64>* bitcast ([32 x i8]* @get_rand_uint64.buf to <4 x i64>*), align 32, !tbaa !9
  //   %9 = add <4 x i64> %7, %6
  //   %10 = add <4 x i64> %6, <i64 7, i64 5, i64 3, i64 1>
  //   %11 = lshr <4 x i64> %5, <i64 1, i64 1, i64 1, i64 1>
  //   %12 = lshr <4 x i64> %9, <i64 3, i64 3, i64 3, i64 3>
  //   %13 = bitcast <4 x i64> %5 to <8 x i32>
  //   %14 = shufflevector <8 x i32> %13, <8 x i32> poison, <8 x i32> <i32 5, i32 6, i32 7, i32 0, i32 1, i32 2, i32 3, i32 4>
  //   %15 = bitcast <8 x i32> %14 to <4 x i64>
  //   %16 = bitcast <4 x i64> %9 to <8 x i32>
  //   %17 = shufflevector <8 x i32> %16, <8 x i32> poison, <8 x i32> <i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1, i32 2>
  //   %18 = bitcast <8 x i32> %17 to <4 x i64>
  //   %19 = add <4 x i64> %11, %15
  //   %20 = add <4 x i64> %12, %18
  //   %21 = xor <4 x i64> %11, %18
  //   store <4 x i64> %19, <4 x i64>* @rng_state.0, align 32, !tbaa !9
  //   store <4 x i64> %10, <4 x i64>* @rng_state.3, align 32, !tbaa !9
  //   store <4 x i64> %20, <4 x i64>* @rng_state.1, align 32, !tbaa !9
  //   store <4 x i64> %21, <4 x i64>* @rng_state.2, align 32, !tbaa !9
  //   br label %22
  // end:                                               ; preds = %4, %0
  //   %23 = phi i32 [ 0, %4 ], [ %1, %0 ]
  //   %24 = shl nsw i32 %23, 3
  //   %25 = sext i32 %24 to i64
  //   %26 = getelementptr inbounds [32 x i8], [32 x i8]* @get_rand_uint64.buf, i64 0, i64 %25
  //   %27 = bitcast i8* %26 to i64*
  //   %28 = load i64, i64* %27, align 8
  //   %29 = add nsw i32 %23, 1
  //   store i32 %29, i32* @get_rand_uint64.i, align 4, !tbaa !10
  //   ret i64 %28
  // }
  // clang-format on
  void insertRandUint64ShishuaCall(IRBuilder<> &Builder, Instruction *I,
                                   Value **rand) {

    int nbBytesRequested = 1;
    if (I->getType()->isVectorTy()) {
      auto *vecTy = static_cast<VectorType *>(I->getType());
#if LLVM_VERSION_MAJOR >= 11
      auto size = vecTy->getElementCount().getKnownMinValue();
#else
      auto size = vecTy->getElementCount();
#endif
      nbBytesRequested = (int)ceil(size / 8.0);
    }

    Module *M = I->getModule();
    const std::string function_name =
        "_shishua_uint64" + std::to_string(nbBytesRequested);
    if (Function *shishua = M->getFunction(function_name)) {
      std::vector<Value *> args = {Builder.getInt32(nbBytesRequested)};
      *rand = Builder.CreateCall(shishua, args);
      return;
    }

    BasicBlock *caller = Builder.GetInsertBlock();
    Function *originalFunction = I->getParent()->getParent();
    Type *int8Ty = Type::getInt8Ty(M->getContext());
    Type *int32Ty = Type::getInt32Ty(M->getContext());
    Type *int64Ty = Type::getInt64Ty(M->getContext());
    Type *floatTy = Type::getFloatTy(M->getContext());
    Type *int32x8Ty = GET_VECTOR_TYPE(int32Ty, 8);
    Type *int64x4Ty = GET_VECTOR_TYPE(int64Ty, 4);

    FunctionType *shishuType = FunctionType::get(int64Ty, {int32Ty}, false);

    Function *shishua = getOrInsertFunction(M, function_name, shishuType);

    Argument *nbBytesRequestedArg = &*shishua->arg_begin();

    errs() << "Inserting shishua call\n";
    errs() << "shishua: " << *shishua << "\n";
    BasicBlock *entryBB =
        BasicBlock::Create(Builder.getContext(), "entry", shishua);
    BasicBlock *rngBB =
        BasicBlock::Create(Builder.getContext(), "rng", shishua);
    BasicBlock *endBB =
        BasicBlock::Create(Builder.getContext(), "end", shishua);

    // entry block
    Builder.SetInsertPoint(entryBB);
    GlobalVariable *rand_uint64_i = getGVRandUint64Counter(*M);
    auto *randUint64ILoad = Builder.CreateLoad(int32Ty, rand_uint64_i);
    randUint64ILoad->setAlignment(Align(4));
    //  %2 = icmp sgt i32 %1, 120
    auto cstICmpSgt = 0;
    switch (nbBytesRequested) {
    case 1:
      cstICmpSgt = 255;
      break;
    case 2:
      cstICmpSgt = 254;
      break;
    case 3:
      cstICmpSgt = 252;
      break;
    case 4:
      cstICmpSgt = 248;
      break;
    default:
      errs() << "Unsupported number of bytes requested: " << nbBytesRequested
             << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
    Value *randUint64ICmp =
        Builder.CreateICmpSGT(randUint64ILoad, Builder.getInt32(cstICmpSgt));
    Builder.CreateCondBr(randUint64ICmp, rngBB, endBB);

    // rng block
    Builder.SetInsertPoint(rngBB);
    GlobalVariable *rng_state0 = getGVRNGState(M, "rng_state.0");
    GlobalVariable *rng_state3 = getGVRNGState(M, "rng_state.3");
    GlobalVariable *rng_state1 = getGVRNGState(M, "rng_state.1");
    GlobalVariable *rng_state2 = getGVRNGState(M, "rng_state.2");

    auto *rngState0Load = Builder.CreateLoad(int64x4Ty, rng_state0);
    auto *rngState3Load = Builder.CreateLoad(int64x4Ty, rng_state3);
    auto *rngState1Load = Builder.CreateLoad(int64x4Ty, rng_state1);
    auto *rngState2Load = Builder.CreateLoad(int64x4Ty, rng_state2);

    rngState0Load->setAlignment(Align(32));
    rngState1Load->setAlignment(Align(32));
    rngState3Load->setAlignment(Align(32));
    rngState2Load->setAlignment(Align(32));

    GlobalVariable *rand_uint64_buffer = getGVRandUint64Buffer(*M);
    Value *bitcast =
        Builder.CreateBitCast(rand_uint64_buffer, int64x4Ty->getPointerTo());
    Builder.CreateStore(rngState2Load, bitcast);
    Value *add1 = Builder.CreateAdd(rngState1Load, rngState3Load);
    Value *cst = getVectorConstant<int64_t>(M, {7, 5, 3, 1});
    Value *add2 = Builder.CreateAdd(rngState3Load, cst);
    Value *cst2 = getVectorConstant<int64_t>(M, {1, 1, 1, 1});
    Value *lshr1 = Builder.CreateLShr(rngState0Load, cst2);
    Value *cst3 = getVectorConstant<int64_t>(M, {3, 3, 3, 3});
    Value *lshr2 = Builder.CreateLShr(add1, cst3);
    Value *bitcast1 = Builder.CreateBitCast(rngState0Load, int32x8Ty);
    Value *shufflePermutation1 =
        getVectorConstant<int32_t>(M, {5, 6, 7, 0, 1, 2, 3, 4});
    Value *shuffle1 = Builder.CreateShuffleVector(
        bitcast1, UndefValue::get(int32x8Ty), shufflePermutation1);
    Value *bitcast2 = Builder.CreateBitCast(shuffle1, int64x4Ty);
    Value *bitcast3 = Builder.CreateBitCast(add1, int32x8Ty);
    Value *shufflePermutation2 =
        getVectorConstant<int32_t>(M, {3, 4, 5, 6, 7, 0, 1, 2});
    Value *shuffle2 = Builder.CreateShuffleVector(
        bitcast3, UndefValue::get(int32x8Ty), shufflePermutation2);
    Value *bitcast4 = Builder.CreateBitCast(shuffle2, int64x4Ty);
    Value *add3 = Builder.CreateAdd(lshr1, bitcast2);
    Value *add4 = Builder.CreateAdd(lshr2, bitcast4);
    Value *xor1 = Builder.CreateXor(lshr1, bitcast4);
    Builder.CreateStore(add3, rng_state0);
    Builder.CreateStore(add2, rng_state3);
    Builder.CreateStore(add4, rng_state1);
    Builder.CreateStore(xor1, rng_state2);
    Builder.CreateBr(endBB);

    // end block
    Builder.SetInsertPoint(endBB);
    PHINode *phiNode = Builder.CreatePHI(int32Ty, 2);
    phiNode->addIncoming(Builder.getInt32(0), rngBB);
    phiNode->addIncoming(randUint64ILoad, entryBB);
    Value *shl = Builder.CreateShl(phiNode, 3, "", false, true);
    Value *sext = Builder.CreateSExt(shl, int64Ty);
    std::vector<Value *> gepIndices = {Builder.getInt64(0), sext};
    ArrayType *arrayType = ArrayType::get(int8Ty, shishua_buffer_size);
    // create a GEP with opaque pointer type
    Value *gep = Builder.CreateGEP(arrayType, rand_uint64_buffer, gepIndices);
    Value *bitcast5 = Builder.CreateBitCast(gep, int64Ty->getPointerTo());
    Value *load = Builder.CreateLoad(int64Ty, bitcast5);
    Value *add5 = Builder.CreateNSWAdd(phiNode, nbBytesRequestedArg);
    auto *store = Builder.CreateStore(add5, rand_uint64_i);
    store->setAlignment(Align(4));
    Builder.CreateRet(load);

    Builder.SetInsertPoint(caller);
    std::vector<Value *> args = {Builder.getInt32(nbBytesRequested)};
    *rand = Builder.CreateCall(shishua, args);
    errs() << "Shishua call inserted\n";
    errs() << "rand: " << **rand << "\n";
  }

  void checkAVX2Support() {
    if (not cpuTargetInfo("+avx2")) {
      errs() << "AVX2 is not supported on this machine\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  bool cpuTargetInfo(const std::string &feature) {
    static bool print = false;
    // Detect the host CPU
    llvm::StringRef CPU = llvm::sys::getHostCPUName();
    if (not print) {
      errs() << "CPU: " << CPU << "\n";
    }
    llvm::SubtargetFeatures Features;
    llvm::StringMap<bool> HostFeatures;
    if (llvm::sys::getHostCPUFeatures(HostFeatures)) {
      for (auto &Feature : HostFeatures) {
        Features.AddFeature(Feature.first(), Feature.second);
      }
    }

    // Check for AVX2 feature
    auto features = Features.getFeatures();
    if (not print) {
      errs() << "Features: ";
      for (auto &f : features) {
        errs() << f << " ";
      }
      errs() << "\n";
    }

    bool HasFeature =
        std::find(features.begin(), features.end(), feature) != features.end();

    if (not print) {
      llvm::outs() << feature << " support: " << (HasFeature ? "Yes" : "No")
                   << "\n";
    }
    print = true;
    return HasFeature;
  }

  void insertRandUint64Call(IRBuilder<> &Builder, Instruction *I,
                            Value **rand) {
    if (VfclibInstRNG == "xoroshiro") {
      insertRandUint64XoroshiroCall(Builder, I, rand);
    } else if (VfclibInstRNG == "shishua") {
      checkAVX2Support();
      insertRandUint64ShishuaCall(Builder, I, rand);
    } else {
      errs() << "Unsupported RNG function: " << VfclibInstRNG << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
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

    Value *rand = nullptr;
    insertRandUint64XoroshiroCall(Builder, I, &rand);

    // Get rand double between 0 and 1
    Value *lshr3 = Builder.CreateLShr(rand, 12);
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

  void insertVectorizeRandUint64Call(IRBuilder<> &Builder, Instruction *I,
                                     Value **rand_uint64) {
    VECTOR_TYPE *fpVT = dyn_cast<VECTOR_TYPE>(I->getType());
    int size = fpVT->getNumElements();
    VECTOR_TYPE *iVT = GET_VECTOR_TYPE(Type::getInt64Ty(I->getContext()), size);
    // create undef value with the same type as the vector
    *rand_uint64 = UndefValue::get(iVT);
    std::vector<Value *> elementsVec;
    Value *rand = nullptr;
    for (int i = 0; i < size; i++) {
      insertRandUint64Call(Builder, I, &rand);
      elementsVec.push_back(rand);
      *rand_uint64 = Builder.CreateInsertElement(*rand_uint64, elementsVec[i],
                                                 Builder.getInt32(i),
                                                 "insert" + std::to_string(i));
    }
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

  GlobalVariable *getGVRNGState(Module &M, const std::string &name) {

    Type *int64Ty = Type::getInt64Ty(M.getContext());
    Type *rngType = nullptr;
    Constant *zero = nullptr;
    if (VfclibInstRNG == "xoroshiro") {
      rngType = int64Ty;
      zero = ConstantInt::get(int64Ty, 0);
    } else if (VfclibInstRNG == "shishua") {
      rngType = GET_VECTOR_TYPE(int64Ty, 4);
      zero = ConstantInt::get(rngType, 0);
    } else {
      errs() << "Unsupported RNG function: " << VfclibInstRNG << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }

    GlobalVariable *rng_state = M.getGlobalVariable(name, true);
    if (rng_state == nullptr) {
      rng_state = new GlobalVariable(
          M,
          /* type */ rngType,
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

  GlobalVariable *getGVRNGState(Module *M, const std::string &name) {
    return getGVRNGState(*M, name);
  }

  GlobalVariable *getGVRandUint64Counter(Module &M) {
    Type *int32Ty = Type::getInt32Ty(M.getContext());
    const std::string name = "shishua_buffer_index";
    GlobalVariable *rand_uint64_i = M.getGlobalVariable(name, true);
    if (rand_uint64_i == nullptr) {
      rand_uint64_i = new GlobalVariable(
          M,
          /* type */ int32Ty,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ ConstantInt::get(int32Ty, 0),
          /* name */
          name,
          /* insertbefore */ nullptr,
          /* threadmode */ GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
    }
    return rand_uint64_i;
  }

  // @get_rand_uint64.buf = internal unnamed_addr global [32 x i8]
  // zeroinitializer, align 32
  GlobalVariable *getGVRandUint64Buffer(Module &M) {
    const std::string name = "shishua_buffer";
    Type *uint8Ty = Type::getInt8Ty(M.getContext());
    ArrayType *arrayType = ArrayType::get(uint8Ty, shishua_buffer_size);
    GlobalVariable *rand_uint64_buf = M.getGlobalVariable(name, true);
    if (rand_uint64_buf == nullptr) {
      rand_uint64_buf = new GlobalVariable(
          M,
          /* type */ arrayType,
          /* isConstant */ false,
          /* linkage */ GlobalValue::InternalLinkage,
          /* initializer */ ConstantAggregateZero::get(arrayType),
          /* name */ name,
          /* insertbefore */ nullptr,
          /* threadmode */ GlobalValue::ThreadLocalMode::GeneralDynamicTLSModel,
          /* addresspace */ 0,
          /* isExternallyInitialized */ false);
      auto align = Align(32);
      rand_uint64_buf->setAlignment(align);
    }
    return rand_uint64_buf;
  }

  Value *insertNextSeed(Module &M, IRBuilder<> &Builder, Value *I) {
    Type *int64Ty = Type::getInt64Ty(M.getContext());
    Constant *nextSeedCst1 = ConstantInt::get(int64Ty, -7046029254386353131);
    Value *add = Builder.CreateAdd(I, nextSeedCst1);
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
    return xor5;
  }

  Value *insertNextSeedVectorize(Module &M, IRBuilder<> &Builder, Value *I) {
    Type *int64Ty = Type::getInt64Ty(M.getContext());
    VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
    auto count = VT->getNumElements();
    auto size = CREATE_VECTOR_ELEMENT_COUNT(count);
    auto *nextSeedCst1 = ConstantInt::get(int64Ty, -7046029254386353131);
    auto *nextSeedCst1Vec = ConstantVector::getSplat(size, nextSeedCst1);
    auto *add = Builder.CreateAdd(I, nextSeedCst1Vec);
    auto *cst30 = ConstantVector::getSplat(size, Builder.getInt64(30));
    auto *lshr1 = Builder.CreateLShr(add, cst30);
    auto *xor3 = Builder.CreateXor(lshr1, add);
    auto *nextSeedCst2 = ConstantInt::get(int64Ty, -4658895280553007687);
    auto *nextSeedCst2Vec = ConstantVector::getSplat(size, nextSeedCst2);
    auto *mul1 = Builder.CreateMul(xor3, nextSeedCst2Vec);
    auto *cst27 = ConstantVector::getSplat(size, Builder.getInt64(27));
    auto *lshr2 = Builder.CreateLShr(mul1, cst27);
    auto *xor4 = Builder.CreateXor(lshr2, mul1);
    auto *nextSeedCst3 = ConstantInt::get(int64Ty, -7723592293110705685);
    auto *nextSeedCst3Vec = ConstantVector::getSplat(size, nextSeedCst3);
    auto *mul2 = Builder.CreateMul(xor4, nextSeedCst3Vec);
    auto *cst31 = ConstantVector::getSplat(size, Builder.getInt64(31));
    auto *lshr3 = Builder.CreateLShr(mul2, cst31);
    auto *xor5 = Builder.CreateXor(lshr3, mul2);
    return xor5;
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
      Value *seed = nullptr;

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
        Value *tvsec = Builder.CreateLoad(int64Ty, TvSecPtr, "tv_sec");

        // Load timeval->tv_usec
        Value *TvUsecPtr =
            Builder.CreateStructGEP(TimevalTy, AllocaTimeval, 1, "tv_usec_ptr");
        Value *tvusec = Builder.CreateLoad(int64Ty, TvUsecPtr, "tv_usec");
        Function *syscallF = getOrCreateSyscallFunction(M);
        Constant *gettidSyscallId = ConstantInt::get(int64Ty, SYS_gettid);
        Value *syscall = Builder.CreateCall(syscallF, gettidSyscallId);
        Value *xor1 = Builder.CreateXor(tvsec, tvusec);
        seed = Builder.CreateXor(xor1, syscall);
      } else {
        seed = ConstantInt::get(int64Ty, VfclibSeed);
      }

      if (VfclibInstRNG == "xoroshiro") {
        Value *nextSeed = insertNextSeed(M, Builder, seed);
        GlobalVariable *rng_state0 = getGVRNGState(M, "rng_state.0");
        Builder.CreateStore(nextSeed, rng_state0);
        nextSeed = insertNextSeed(M, Builder, nextSeed);
        GlobalVariable *rng_state1 = getGVRNGState(M, "rng_state.1");
        Builder.CreateStore(nextSeed, rng_state1);
      } else if (VfclibInstRNG == "shishua") {

        Value *nextSeed1 = insertNextSeed(M, Builder, seed);
        Value *nextSeed2 = insertNextSeed(M, Builder, nextSeed1);
        Value *nextSeed3 = insertNextSeed(M, Builder, nextSeed2);
        Value *nextSeed4 = insertNextSeed(M, Builder, nextSeed3);
        // create a vector with the 4 seeds
        VECTOR_TYPE *VT = GET_VECTOR_TYPE(int64Ty, 4);
        Value *seeds = UndefValue::get(VT);
        seeds =
            Builder.CreateInsertElement(seeds, nextSeed1, Builder.getInt32(0));
        seeds =
            Builder.CreateInsertElement(seeds, nextSeed2, Builder.getInt32(1));
        seeds =
            Builder.CreateInsertElement(seeds, nextSeed3, Builder.getInt32(2));
        seeds =
            Builder.CreateInsertElement(seeds, nextSeed4, Builder.getInt32(3));

        GlobalVariable *rng_state0 = getGVRNGState(M, "rng_state.0");
        Builder.CreateStore(seeds, rng_state0);
        seeds = insertNextSeedVectorize(M, Builder, seeds);
        GlobalVariable *rng_state1 = getGVRNGState(M, "rng_state.1");
        Builder.CreateStore(seeds, rng_state1);
        GlobalVariable *rng_state2 = getGVRNGState(M, "rng_state.2");
        seeds = insertNextSeedVectorize(M, Builder, seeds);
        Builder.CreateStore(seeds, rng_state2);
        GlobalVariable *rng_state3 = getGVRNGState(M, "rng_state.3");
        seeds = insertNextSeedVectorize(M, Builder, seeds);
        Builder.CreateStore(seeds, rng_state3);
      }

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

  Value *getOrCreateUpDownFunction(IRBuilder<> &Builder, Instruction *I,
                                   Fops opCode, std::set<User *> &users) {
    Module *M = Builder.GetInsertBlock()->getParent()->getParent();
    Type *srcTy = I->getType();

    std::string function_name =
        getFunctionName(opCode) + "_" + getTypeName(srcTy) + "_updown";

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
      Value *sr_op = createUpDownOp(Builder, I, &*args, opCode, users);
      errs() << "sr_op: " << *sr_op << "\n";
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

  Value *replaceArithmeticWithMCACall_UpOrDown(IRBuilder<> &Builder,
                                               Instruction *I,
                                               std::set<User *> &users) {
    Fops opCode = mustReplace(*I);
    Value *value = getOrCreateUpDownFunction(Builder, I, opCode, users);
    return value;
  }

  Value *insertOriginalInstruction(IRBuilder<> &Builder, Instruction *I,
                                   Fops opCode, Function::arg_iterator args) {
    Value *op = nullptr;
    Value *arg1 = nullptr, *arg2 = nullptr, *arg3 = nullptr;
    switch (opCode) {
    case Fops::FOP_ADD:
      arg1 = &*args;
      args++;
      arg2 = &*args;
      op = Builder.CreateFAdd(arg1, arg2);
      break;
    case Fops::FOP_SUB:
      arg1 = &*args;
      args++;
      arg2 = &*args;
      op = Builder.CreateFSub(arg1, arg2);
      break;
    case Fops::FOP_MUL:
      arg1 = &*args;
      args++;
      arg2 = &*args;
      op = Builder.CreateFMul(arg1, arg2);
      break;
    case Fops::FOP_DIV:
      arg1 = &*args;
      args++;
      arg2 = &*args;
      op = Builder.CreateFDiv(arg1, arg2);
      break;
    case Fops::FOP_FMA:
      // arg1 = &*args;
      // args++;
      // arg2 = &*args;
      // args++;
      // arg3 = &*args;
      op = CREATE_FMA_CALL(Builder, arg1->getType(), args);
      break;
    default:
      errs() << "Unsupported opcode: " << opCode << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
    return op;
  }

  // clang-format off
  // define dso_local <4 x float> @ud_round_b32_4x(<4 x float> noundef %0) local_unnamed_addr #10 {
  // entry: 
  //   %2 = fcmp une <4 x float> %0, zeroinitializer
  //   %3 = bitcast <4 x i1> %2 to i4
  //   %4 = icmp eq i4 %3, 0
  //   br i1 %4, label %15, label %5
  // noise:                                                ; preds = entry
  //   %6 = bitcast <4 x float> %0 to <4 x i32>
  //   %7 = tail call i32 @get_rand_uint32()
  //   %8 = insertelement <4 x i32> poison, i32 %7, i64 0
  //   %9 = shufflevector <4 x i32> %8, <4 x i32> poison, <4 x i32> zeroinitializer
  //   %10 = and <4 x i32> %9, <i32 1, i32 2, i32 4, i32 8>
  //   %11 = icmp eq <4 x i32> %10, zeroinitializer
  //   %12 = select <4 x i1> %11, <4 x i32> <i32 -1, i32 -1, i32 -1, i32 -1>, <4 x i32> <i32 1, i32 1, i32 1, i32 1>
  //   %13 = add <4 x i32> %12, %6
  //   %14 = bitcast <4 x i32> %13 to <4 x float>
  //   br label %15
  // ret:                                               ; preds = entry, noise
  //   %16 = phi <4 x float> [ %14, %5 ], [ %0, %1 ]
  //   ret <4 x float> %16
  // }
  // clang-format on
  Value *createUpDownOp(IRBuilder<> &Builder, Instruction *I,
                        Function::arg_iterator args, Fops opCode,
                        std::set<User *> &users) {

    Type *srcTy = nullptr;
    bool isVector = I->getType()->isVectorTy();
    VECTOR_TYPE *VT = dyn_cast<VECTOR_TYPE>(I->getType());
    srcTy = (isVector) ? VT->getElementType() : I->getType();
    Type *fpAsIntTy = getFPAsIntType(srcTy);

    BasicBlock *entryBB = Builder.GetInsertBlock();
    BasicBlock *noiseBB =
        BasicBlock::Create(I->getContext(), "noise", entryBB->getParent());
    BasicBlock *retBB =
        BasicBlock::Create(I->getContext(), "ret", entryBB->getParent());

    // entry block
    Builder.SetInsertPoint(entryBB);
    Instruction *op = static_cast<Instruction *>(
        insertOriginalInstruction(Builder, I, opCode, args));

    uint32_t size = 1;
    auto count = CREATE_VECTOR_ELEMENT_COUNT(size);

    Value *randomBits = nullptr, *zeroinitializerInt = nullptr,
          *zeroinitializerInt64 = nullptr, *zeroinitializerFP = nullptr,
          *cmp = nullptr, *bitcast = nullptr, *icmp = nullptr;
    Type *bitCastTy = nullptr;

    if (isVector) {
      // entry:
      //   %2 = fcmp une <4 x float> %0, zeroinitializer
      //   %3 = bitcast <4 x i1> %2 to i4
      //   %4 = icmp eq i4 %3, 0
      //   br i1 %4, label %15, label %5
      srcTy = VT->getElementType();
      size = VT->getNumElements();
      count = CREATE_VECTOR_ELEMENT_COUNT(size);
      auto *vecIntTy = VECTOR_TYPE::get(fpAsIntTy, size);
      zeroinitializerFP = ConstantAggregateZero::get(I->getType());
      zeroinitializerInt = ConstantAggregateZero::get(vecIntTy);
      zeroinitializerInt64 = ConstantAggregateZero::get(
          VECTOR_TYPE::get(Builder.getInt64Ty(), size));
      //   %2 = fcmp une <4 x float> %0, zeroinitializer
      cmp = Builder.CreateFCmpUNE(op, zeroinitializerFP, "is_zero");
      auto *intNTy = Type::getIntNTy(I->getContext(), VT->getNumElements());
      //   %3 = bitcast <4 x i1> %2 to i4
      bitcast = Builder.CreateBitCast(cmp, intNTy);
      errs() << "bitcast: " << *bitcast << "\n";
      errs() << "zeroinitializerInt: " << *zeroinitializerInt << "\n";
      // Generate zeronitializer for bitcast
      auto *zero = ConstantInt::get(intNTy, 0);
      //   %4 = icmp eq i4 %3, 0
      errs() << "zero: " << *zero << "\n";
      icmp = Builder.CreateICmpEQ(bitcast, zero);
      Builder.CreateCondBr(icmp, retBB, noiseBB);
      bitCastTy = VECTOR_TYPE::get(fpAsIntTy, VT->getNumElements());
    } else {
      srcTy = I->getType();
      zeroinitializerFP = ConstantFP::get(srcTy, 0.0);
      zeroinitializerInt = ConstantInt::get(fpAsIntTy, 0);
      zeroinitializerInt64 = ConstantInt::get(Builder.getInt64Ty(), 0);
      Constant *zeroFP = ConstantFP::get(I->getType(), 0.0);
      cmp = Builder.CreateFCmpOEQ(op, zeroFP, "is_zero");
      Builder.CreateCondBr(cmp, retBB, noiseBB);
      bitCastTy = fpAsIntTy;
    }

    // noise block
    Builder.SetInsertPoint(noiseBB);
    Value *fpAsInt = Builder.CreateBitCast(op, bitCastTy, "fpAsInt");
    users.insert(static_cast<User *>(fpAsInt));

    // if (isVector and VfclibInstRNG == "xoroshiro") {
    //   insertVectorizeRandUint64Call(Builder, op, &randomBits);
    // } else {
    // }
    insertRandUint64Call(Builder, op, &randomBits);

    if (isVector) {
      // clang-format off
      // % 8 = insertelement<4 x i32> poison, i32 % 7, i64 0 
      // % 9 = shufflevector<4 x i32> % 8, <4 x i32> poison, <4 x i32> zeroinitializer 
      // % 10 = and<4 x i32> % 9, <i32 1, i32 2, i32 4, i32 8>
      // clang-format on
      // if (srcTy->isFloatTy()) {
      //   randomBits = Builder.CreateBitCast(randomBits, fpAsIntTy,
      //   "randBits_to_fp");
      // }
      auto *vecTy = VECTOR_TYPE::get(Builder.getInt64Ty(), size);
      auto *poison = UndefValue::get(vecTy);
      auto *zero = Builder.getInt64(0);
      auto *insert =
          Builder.CreateInsertElement(poison, randomBits, zero, "insertZero");
      auto *shuffle =
          Builder.CreateShuffleVector(insert, poison, zeroinitializerInt);
      std::vector<uint64_t> pow2;
      for (unsigned i = 0; i < VT->getNumElements(); i++) {
        pow2.push_back(1 << i);
      }
      Module *M = Builder.GetInsertBlock()->getParent()->getParent();
      auto *cstPow2 = getVectorConstant<uint64_t>(M, pow2);
      randomBits = Builder.CreateAnd(shuffle, cstPow2);
    } else {
      // %6 = and i32 %5, 1
      randomBits = Builder.CreateAnd(randomBits, 1);
    }

    errs() << "randomBits: " << *randomBits << "\n";
    errs() << "zeroinitializerInt64: " << *zeroinitializerInt64 << "\n";
    auto *zero = ConstantInt::get(randomBits->getType(), 0);
    auto *icmp2 = Builder.CreateICmpEQ(randomBits, zero);

    Constant *one = ConstantInt::get(fpAsIntTy, 1);
    Constant *mone = ConstantInt::get(fpAsIntTy, -1);
    if (isVector) {
      one = ConstantVector::getSplat(count, one);
      mone = ConstantVector::getSplat(count, mone);
    }

    auto *select = Builder.CreateSelect(icmp2, mone, one);
    auto *add2 = Builder.CreateAdd(fpAsInt, select);
    auto *fpNoised = Builder.CreateBitCast(add2, I->getType());
    Builder.CreateBr(retBB);

    // ret block
    Builder.SetInsertPoint(retBB);
    PHINode *phiNode = Builder.CreatePHI(I->getType(), 2);
    phiNode->addIncoming(fpNoised, noiseBB);
    phiNode->addIncoming(op, entryBB);

    return phiNode;
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
              errs() << "Replacing " << *from << " with " << *to << " in "
                     << *ii << '\n';
              op.set(to);
            }
          }
        }
      }
    }
  }

  void insertShishuaCall(Module &M, Instruction *I) {
    auto *int32Ty = Type::getInt32Ty(M.getContext());
    auto *int64Ty = Type::getInt64Ty(M.getContext());
    auto *function_name = "_shishua_uint641";
    FunctionType *shishuType = FunctionType::get(int64Ty, {int32Ty}, false);

    Function *shishua = getOrInsertFunction(M, function_name, shishuType);
    IRBuilder<> Builder(I->getContext());
    Builder.SetInsertPoint(I);
    std::vector<Value *> args = {Builder.getInt32(1)};
    auto *call = Builder.CreateCall(shishua, args);
    I->replaceAllUsesWith(call);
  }

  void replaceArgsWithShishuaInfo(Module &M, Instruction *I) {
    //   call void @print_buffer_(i32 noundef %1, i8* noundef getelementptr
    //   inbounds ([256 x i8], [256 x i8]* @buf, i64 0, i64 0))
    IRBuilder<> Builder(I->getContext());
    Builder.SetInsertPoint(I);
    // get @shishua_buffer
    GlobalVariable *rand_uint64_buffer = getGVRandUint64Buffer(M);
    // get @shishua_buffer_size
    GlobalVariable *rand_uint64_buffer_size = getGVRandUint64Counter(M);
    auto *call = static_cast<CallInst *>(I);
    // replace the first argument with @shishua_buffer_size
    // dereference the pointer
    auto *int32Ty = Type::getInt32Ty(M.getContext());
    auto *load = Builder.CreateLoad(int32Ty, rand_uint64_buffer_size);
    call->setArgOperand(0, load);
    // replace the second argument with @shishua_buffer
    // cast the pointer to uint8_t*
    auto *int8PtrTy = Type::getInt8PtrTy(M.getContext());
    // i8* noundef getelementptr inbounds ([256 x i8], [256 x i8]* @buf, i64 0,
    // i64 0)
    auto *zero = Builder.getInt64(0);
    auto *zero2 = Builder.getInt64(0);
    std::vector<Value *> gep_args = {zero, zero2};
    Type *type = rand_uint64_buffer->getType()->getPointerElementType();
    errs() << "type: " << *type << '\n';
    errs() << "rand_uint64_buffer: " << *rand_uint64_buffer << '\n';
    auto *gep = Builder.CreateGEP(type, rand_uint64_buffer, gep_args);
    call->setArgOperand(1, gep);
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {
    bool modified = false;
    std::set<std::pair<Instruction *, Fops>> WorkList;
    for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
      Instruction &I = *ii;

      if (CallInst *CI = dyn_cast<CallInst>(&I)) {
        Function *F = CI->getCalledFunction();
        if (F->getName() == "CALL_SHISHUA") {
          insertShishuaCall(M, CI);
          continue;
        } else if (F->getName() == "print_shishua_info") {
          replaceArgsWithShishuaInfo(M, CI);
          continue;
        }
      }

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
      if (value != nullptr) {
        I->replaceAllUsesWith(value);
        I->eraseFromParent();
        modified = true;
      }
      // if (value != nullptr and VfclibInstMode != "up-down") {
      //   I->replaceAllUsesWith(value);
      //   I->eraseFromParent();
      //   modified = true;
      // } else if (value != nullptr and VfclibInstMode == "up-down") {
      //   // We need to replace the original instruction with the noised
      //   // instruction for all the users of the original instruction after
      //   the
      //   // noise is added
      //   replaceUsageWith(fp_users, I, value);
      //   modified = true;
      // }
    }

    return modified;
  }
}; // namespace
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass",
                                  false, false);
