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
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#pragma GCC diagnostic pop

#include <cxxabi.h>
#include <fstream>
#include <functional>
#include <regex>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include <interflop-stdlib/common/float_const.h>
#include <interflop-stdlib/common/float_utils.h>

#if LLVM_VERSION_MAJOR < 11
#define GET_VECTOR_TYPE(ty, size) VectorType::get(ty, size)
#else
#define GET_VECTOR_TYPE(ty, size) FixedVectorType::get(ty, size)
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

/* pointer that hold the vfcwrapper Module */
// static Module *vfcwrapperM = nullptr;

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo
enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_CMP, FOP_IGNORE };

// Each instruction can be translated to a string representation
const std::string Fops2str[] = {"add", "sub", "mul", "div", "cmp", "ignore"};

/* valid floating-point type to instrument */
std::map<Type::TypeID, std::string> validTypesMap = {
    std::pair<Type::TypeID, std::string>(Type::FloatTyID, "float"),
    std::pair<Type::TypeID, std::string>(Type::DoubleTyID, "double")};

/* valid vector sizes to instrument */
const std::set<unsigned> validVectorSizes = {2, 4, 8, 16};

struct VfclibInst : public ModulePass {
  static char ID;

  VfclibInst() : ModulePass(ID) {}

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

  /* Load vfcwrapper.ll Module */
  // void loadVfcwrapperIR(Module &M) {
  //   SMDiagnostic err;
  //   std::unique_ptr<Module> _M =
  //       parseIRFile(VfclibInstVfcwrapper, err, M.getContext());
  //   if (_M.get() == nullptr) {
  //     err.print(VfclibInstVfcwrapper.c_str(), errs());
  //     report_fatal_error("libVFCInstrument fatal error");
  //   }
  //   vfcwrapperM = _M.release();
  // }

  bool runOnModule(Module &M) {
    bool modified = false;

    // loadVfcwrapperIR(M);

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
    // runOnModule must return true if the pass modifies the IR
    return modified;
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

  /* Check if Instruction I is a valid instruction to replace */
  bool isValidInstruction(Instruction *I) {
    Type *opType = I->getOperand(0)->getType();
    if (opType->isVectorTy()) {
      return false;
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

  Value *insertRNGCall(IRBuilder<> &Builder, Instruction *I) {
    Module *M = I->getModule();
    std::string typestring = validTypesMap[I->getType()->getTypeID()];
    std::string randName = "get_rand_" + typestring;
    Type *fpAsIntTy = getFPAsIntType(I->getType());
    FunctionType *funcType =
        FunctionType::get(fpAsIntTy, {I->getType()}, false);
    FunctionCallee RNG = M->getOrInsertFunction(randName, funcType);
    return Builder.CreateCall(RNG, I);
  }

  Type *getFPAsIntType(Type *type) {
    if (type->isFloatTy()) {
      return Type::getInt32Ty(type->getContext());
    } else if (type->isDoubleTy()) {
      return Type::getInt64Ty(type->getContext());
    } else {
      errs() << "Unsupported type: " << *type << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Constant *getUIntValue(Type *type, int value) {
    Type *intTy = getFPAsIntType(type);
    if (type->isFloatTy() or type->isDoubleTy()) {
      return ConstantInt::get(intTy, value);
    } else {
      errs() << "Unsupported type: " << *type << "\n";
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
    }
  }

  Value *insertSROpCall(IRBuilder<> &Builder, Instruction *I) {
    Module *M = I->getModule();
    std::string typestring = validTypesMap[I->getType()->getTypeID()];
    std::string srName = sr_hook_name(*I) + "_" + typestring;
    FunctionType *funcType =
        FunctionType::get(I->getType(), {I->getType(), I->getType()}, false);
    FunctionCallee SR = M->getOrInsertFunction(srName, funcType);
    return Builder.CreateCall(SR, {I->getOperand(0), I->getOperand(1)});
  }

  Value *replaceArithmeticWithMCACall_SR(IRBuilder<> &Builder, Instruction *I,
                                         std::set<User *> &users) {
    Type *fpAsIntTy = getFPAsIntType(I->getType());
    Value *value = insertSROpCall(Builder, I);
    return value;
  }

  Value *replaceArithmeticWithMCACall_UpOrDown(IRBuilder<> &Builder,
                                               Instruction *I,
                                               std::set<User *> &users) {
    Type *fpAsIntTy = getFPAsIntType(I->getType());
    Value *randomBit = insertRNGCall(Builder, I);
    users.insert(static_cast<User *>(randomBit));
    Value *FPAsInt = Builder.CreateBitCast(I, fpAsIntTy, "fpAsInt");
    users.insert(static_cast<User *>(FPAsInt));
    // Modify the result of the floating point operation.
    Value *newResult = Builder.CreateAdd(FPAsInt, randomBit, "add_noise");
    Value *newFP = Builder.CreateBitCast(newResult, I->getType(), "new_fp");
    return newFP;
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

  Value *getExponentMask(Type *fpTy) {
    if (fpTy->isFloatTy()) {
      // Extract the exponent from the float
      return getUIntValue(fpTy, FLOAT_GET_EXP);
    } else if (fpTy->isDoubleTy()) {
      // Extract the exponent from the double
      return getUIntValue(fpTy, DOUBLE_GET_EXP);
    } else {
      errs() << "Unsupported type: " << *fpTy << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Constant *getExponentBias(Type *fpTy) {
    if (fpTy->isFloatTy()) {
      return ConstantInt::get(fpTy, FLOAT_EXP_COMP);
    } else if (fpTy->isDoubleTy()) {
      return ConstantInt::get(fpTy, DOUBLE_EXP_COMP);
    } else {
      errs() << "Unsupported type: " << *fpTy << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }
  }

  Value *createGetExponent(IRBuilder<> &Builder, Instruction *I,
                           std::set<User *> &users) {
    // Get the exponent of the floating point number
    Type *srcTy = I->getType();
    Type *dstTy = getFPAsIntType(srcTy);
    Value *fpAsInt = Builder.CreateBitCast(I, dstTy, "fp_as_int");
    Value *exponent_mask = getExponentMask(srcTy);
    Value *BiasedExponentValue =
        Builder.CreateAnd(fpAsInt, exponent_mask, "get_exponent_bits");
    const unsigned mantissa_bitsize = srcTy->getFPMantissaWidth() - 1;
    Value *ShiftExponentBitsValue = Builder.CreateLShr(
        BiasedExponentValue, mantissa_bitsize, "shift_exponent_bits");
    Constant *exponentBias = getExponentBias(srcTy);
    Value *ExponentValue =
        Builder.CreateSub(ShiftExponentBitsValue, exponentBias, "get_exponent");
    return ExponentValue;
  }

  Value *createGetPredecessor(IRBuilder<> &Builder, Instruction *I, std::set<User*> &users) {
      // Get the predecessor of the floating point number
      Type *srcTy = I->getType();
      Type *dstTy = getFPAsIntType(srcTy);
      Value *fpAsInt = Builder.CreateBitCast(I, dstTy, "fp_as_int");
      
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
    case Instruction::FCmp:
      return FOP_IGNORE;
    default:
      return FOP_IGNORE;
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
      // errs() << "After instrumentation: " << B << "\n";
      if (value != nullptr) {
        for (User *user : I->users()) {
          if (fp_users.find(user) == fp_users.end()) {
            user->replaceUsesOfWith(I, value);
          }
        }
      }
      // errs() << "After replacement: " << B << "\n";
      modified = true;
    }

    return modified;
  }
}; // namespace
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass",
                                  false, false);
