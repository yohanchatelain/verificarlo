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
#include "llvm/Linker/Linker.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/MC/SubtargetFeature.h>
// #include <llvm/Support/Alignment.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
// #include <llvm/Target/TargetRegistry.h>
#if LLVM_VERSION_MAJOR >= 11
#undef PIC
#include <llvm/MC/TargetRegistry.h>
#else
#include <llvm/Support/TargetRegistry.h>
#endif
#include "llvm/IR/Mangler.h"
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
#define GET_VECTOR_ELEMENT_COUNT(vecType) vecType->getNumElements()
#else
#define VECTOR_TYPE FixedVectorType
#define GET_VECTOR_TYPE(ty, size) FixedVectorType::get(ty, size)
#define CREATE_FMA_CALL(Builder, type, args)                                   \
  Builder.CreateIntrinsic(Intrinsic::fma, type, args)
#define CREATE_VECTOR_ELEMENT_COUNT(size) ElementCount::getFixed(size)
#define GET_VECTOR_ELEMENT_COUNT(vecType)                                      \
  ((::llvm::FixedVectorType *)vecType)->getNumElements()
#endif

using namespace llvm;
// VfclibInst pass command line arguments

enum debugOptions {
  GetOperands = 1 << 0,
  TargetFeatures = 1 << 1,
  ABI = 1 << 2
};

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

static cl::opt<std::string> VfclibInstStoIRFile(
    "vfclibinst-sr-file",
    cl::desc("Name of the IR file that contains the stochastic operators"),
    cl::value_desc("SRIRFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

const std::string up_down_mode = "up-down";
const std::string up_down_ns = "up";
const std::string stochastic_rounding_mode = "stochastic-rounding";
const std::string stochastic_rounding_ns = "sr";

static cl::opt<std::string> VfclibInstMode(
    "vfclibinst-mode",
    cl::desc("Instrumentation mode: up-down or stochastic-rounding"),
    cl::value_desc("Mode"), cl::init(up_down_mode));

static cl::opt<std::string>
    VfclibInstDispatch("vfclibinst-dispatch",
                       cl::desc("Instrumentation dispatch: static or dynamic"),
                       cl::value_desc("Dispatch"), cl::init("dynamic"));

static cl::opt<bool>
    VfclibInstInstrumentFMA("vfclibinst-inst-fma",
                            cl::desc("Instrument floating point fma"),
                            cl::value_desc("InstrumentFMA"), cl::init(false));

static cl::opt<bool> VfclibInstDebug("vfclibinst-debug",
                                     cl::desc("Activate debug mode"),
                                     cl::value_desc("Debug"), cl::init(false));

static cl::bits<debugOptions> VfclibInstDebugOptions(
    cl::desc("Debug options"),
    cl::values(
        clEnumValN(debugOptions::GetOperands, "vfclibinst-debug-getoperands",
                   "Debug GetOperands"),
        clEnumValN(debugOptions::TargetFeatures,
                   "vfclibinst-debug-targetfeatures", "Debug TargetFeatures"),
        clEnumValN(debugOptions::ABI, "vfclibinst-debug-abi", "Debug ABI")),
    cl::CommaSeparated);

namespace {

// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo
enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_FMA, FOP_CMP, FOP_IGNORE };

// Each instruction can be translated to a string representation
const std::string Fops2str[] = {"add", "sub", "mul",   "div",
                                "fma", "cmp", "ignore"};

/* valid floating-point type to instrument */
std::map<Type::TypeID, std::string> validTypesMap = {
    std::pair<Type::TypeID, std::string>(Type::FloatTyID, "f32"),
    std::pair<Type::TypeID, std::string>(Type::DoubleTyID, "f64")};

/* valid vector sizes to instrument */
const std::set<unsigned> validVectorSizes = {2, 4, 8, 16, 32, 64};

std::map<std::string, std::string> demangledNamesToMangled;
std::map<std::string, std::string> demangledShortNamesToMangled;
std::set<std::string> functionsToExclude;

struct VfclibInst : public ModulePass {
  static char ID;
  std::unique_ptr<Module> stoLibModule;

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
  static std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
    }
    return tokens;
  }

  static std::vector<std::string> split(const StringRef &s, char delimiter) {
    return split(s.str(), delimiter);
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

  /* Load vfcwrapper.ll Module */
  std::unique_ptr<Module> loadVfcwrapperIR(Module &M,
                                           const std::string &irFile) {
    SMDiagnostic err;
    auto newM = parseIRFile(irFile, err, M.getContext());
    if (newM.get() == nullptr) {
      err.print(irFile.c_str(), errs());
      report_fatal_error("libVFCInstrumentPRISM fatal error");
    }
    return newM;
  }

  void getDemangledNamesLibSR(Module *M) {
    for (auto &F : M->functions()) {
      if (F.isDeclaration()) {
        continue;
      }
      const std::string &mangled_name = F.getName().str();
      functionsToExclude.insert(mangled_name);

      std::string demangled_name = get_demangled_name(mangled_name);
      demangledNamesToMangled[demangled_name] = mangled_name;

      size_t parenPos = demangled_name.find('(');
      std::string demangled_name_short =
          (parenPos != std::string::npos) ? demangled_name.substr(0, parenPos)
                                          : demangled_name;

      demangledShortNamesToMangled[demangled_name_short] = mangled_name;
    }
  }

  void printDemangledNamesLibsSR() {
    errs() << "Demangled names\n";
    for (auto &p : demangledNamesToMangled) {
      errs() << p.first << " : " << p.second << "\n";
    }
  }

  void printDemangledNamesLibsSRShort() {
    errs() << "Short demangled names\n";
    for (auto &p : demangledShortNamesToMangled) {
      errs() << p.first << " : " << p.second << "\n";
    }
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    stoLibModule = loadVfcwrapperIR(M, VfclibInstStoIRFile);
    // if ir is null, an error message has already been printed
    if (stoLibModule.get() == nullptr) {
      report_fatal_error(
          "libVFCInstrumentPRISM fatal error while reading SR library IR");
    }
    getDemangledNamesLibSR(stoLibModule.get());

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

      // Function in the sr library to exclude
      if (functionsToExclude.find(name) != functionsToExclude.end()) {
        continue;
      }

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

  /* Check if Instruction I is a valid instruction to replace; vector case */
  bool isValidVectorInstruction(Type *opType) {
    if (opType == nullptr) {
      errs() << "Unsupported operand type\n";
    }
    auto vecType = static_cast<VectorType *>(opType);
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

  Function *createFunction(FunctionType *functionType,
                           GlobalValue::LinkageTypes linkage, Module &M,
                           const std::string &name) {
#if LLVM_VERSION_MAJOR < 9
    return Function::Create(functionType, Function::ExternalLinkage, name, &M);
#else
    return Function::Create(functionType, Function::ExternalLinkage, name, M);
#endif
  }

  std::string get_mangled_name(Function *F) {
    // Create a Mangler
    llvm::Mangler Mang;

    // Get the mangled name
    std::string MangledName;
    llvm::raw_string_ostream RawOS(MangledName);
    Mang.getNameWithPrefix(RawOS, F, false);
    return MangledName;
  }

  std::string get_demangled_name(const std::string &name) {
    int status;
    char *demangled = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
    if (status == 0) {
      std::string demangled_name(demangled);
      free(demangled);
      return demangled_name;
    } else {
      return name;
    }
  }

  std::string getFPTypeName(Type *Ty) {
    if (Ty->isVectorTy()) {
      auto vecType = static_cast<VectorType *>(Ty);
      auto baseType = vecType->getScalarType();
      auto size = GET_VECTOR_ELEMENT_COUNT(vecType);
      auto base_type_name = validTypesMap[baseType->getTypeID()];
      return base_type_name + "x" + std::to_string(size);
    } else {
      return validTypesMap[Ty->getTypeID()];
    }
  }

  std::string getFunctionName(Instruction *I, Fops opCode,
                              const std::string &dispatch) {
    if (opCode == FOP_IGNORE) {
      errs() << "Unsupported opcode: " << opCode << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }

    auto baseType = I->getType();
    std::string mode;
    if (VfclibInstMode == up_down_mode)
      mode = up_down_ns;
    else if (VfclibInstMode == stochastic_rounding_mode)
      mode = stochastic_rounding_ns;
    else {
      errs() << "Invalid mode: " << VfclibInstMode << "\n";
      report_fatal_error("libVFCInstrumentPRISM fatal error");
    }

    const std::string &opname = Fops2str[opCode];
    const std::string &fpname = getFPTypeName(baseType);

    std::string vectype;
    std::string dispatch_ext = "";

    if (baseType->isVectorTy()) {
      vectype = "vector";
      if (dispatch == "static") {
        dispatch_ext = "_static";
      } else if (dispatch == "dynamic") {
        dispatch_ext = "_dynamic";
      } else {
        errs() << "Invalid dispatch: " << dispatch << "\n";
        llvm_unreachable("Invalid dispatch");
      }
    } else {
      vectype = "scalar";
      dispatch_ext = "";
    }
    return "prism::" + mode + "::" + vectype + "::" + opname + fpname +
           dispatch_ext;
  }

  Function *getFunction(Module *M, StringRef name) {
    if (demangledShortNamesToMangled.find(name.str()) ==
        demangledShortNamesToMangled.end()) {
      return nullptr;
    }

    return M->getFunction(demangledShortNamesToMangled[name.str()]);
  }

  Function *getPRFunction(Instruction *I) {
    Fops opCode = mustReplace(*I);
    const std::string dispatch = VfclibInstDispatch;

    std::string functionName = getFunctionName(I, opCode, dispatch);
    auto function = getFunction(stoLibModule.get(), functionName);
    if (function == nullptr and dispatch == "static") {
      /* Use dynamic implementation if static not version not found */
      /* Useful when vector size is not supported */
      /* by the current architecture */
      functionName = getFunctionName(I, opCode, "dynamic");
      function = getFunction(stoLibModule.get(), functionName);
    }

    if (function == nullptr) {
      if (VfclibInstDebug) {
        errs() << "Function not found: " << functionName << "\n";
        errs() << "Skipping instruction: " << *I << "\n";
      }
      return nullptr;
    }

    // Copy the function into the current module
    auto functionNameMangled = demangledShortNamesToMangled[functionName];
    auto F = I->getModule()->getOrInsertFunction(functionNameMangled,
                                                 function->getFunctionType(),
                                                 function->getAttributes());

    auto parent = I->getParent()->getParent();

#if LLVM_VERSION_MAJOR < 9
    auto f = dyn_cast<Function>(F);
    auto prism_function = dyn_cast<Function>(F);
#else
    auto f = dyn_cast<Function>(F.getCallee());
    auto prism_function = dyn_cast<Function>(F.getCallee());
#endif

    return prism_function;
  }

  bool isDoubleFunction(Function *F) {
    const auto name = F->getName().str();
    return name.find("f64") != std::string::npos;
  }

  bool isf32x2Value(Value *V) {
    if (not V->getType()->isVectorTy()) {
      return false;
    }
    auto vecType = static_cast<VectorType *>(V->getType());
    auto baseType = vecType->getScalarType();
    auto size = GET_VECTOR_ELEMENT_COUNT(vecType);
    return baseType->isFloatTy() and size == 2;
  }

  Value *f32x2ToDoubleCast(IRBuilder<> &Builder, Value *V) {
    auto cast = Builder.CreateBitCast(V, Builder.getDoubleTy());
    return cast;
  }

  std::size_t getArity(Instruction *I) {
    switch (getOpCode(*I)) {
    case FOP_ADD:
    case FOP_SUB:
    case FOP_MUL:
    case FOP_DIV:
      return 2;
    case FOP_FMA:
      return 3;
    default:
      return 0;
    }
  }

  bool is_dynamic_dispatch(const Function *F) {
    const auto name = F->getName().str();
    return name.find("_dynamic") != std::string::npos;
  }

  bool is_static_dispatch(const Function *F) {
    const auto name = F->getName().str();
    return name.find("_static") != std::string::npos;
  }

  std::vector<Value *> getOperands(IRBuilder<> &Builder, Instruction *I,
                                   Function *F, bool isDynamicDispatch) {
    if (VfclibInstDebugOptions.isSet(debugOptions::GetOperands)) {
      errs() << "Get operands for function: " << F->getName() << "\n";
      errs() << *F << "\n";
    }
    std::vector<Value *> operands;
    for (unsigned i = 0; i < getArity(I); i++) {
      auto op = I->getOperand(i);
      auto arg_type = F->getFunctionType()->getParamType(i);

      if (VfclibInstDebugOptions.isSet(debugOptions::GetOperands)) {
        errs() << "Operand " << i << ": " << *op << "\n";
        errs() << "Arg type " << i << ": " << *arg_type << "\n";
      }

      if (isDynamicDispatch and arg_type->isPointerTy()) {

        // dynamic dispatch with vector arguments
        // auto alloca = Builder.CreateAlloca(op->getType());
        // auto store = Builder.CreateStore(op, alloca, true);
        // operands.push_back(alloca);

        // dynamic dispatch with pointer arguments
        // if the vector is passed as a value
        // need to create an alloca and store the value
        auto alloca = Builder.CreateAlloca(op->getType());
        auto store = Builder.CreateStore(op, alloca, true);
        auto bitcast = Builder.CreateBitCast(alloca, arg_type);
        operands.push_back(bitcast);

        if (VfclibInstDebugOptions.isSet(debugOptions::GetOperands))
          errs() << "Operand " << i << " after alloca: " << *alloca << "\n";

      } else if (isf32x2Value(op)) {
        op = f32x2ToDoubleCast(Builder, op);
        operands.push_back(op);

        if (VfclibInstDebugOptions.isSet(debugOptions::GetOperands))
          errs() << "Operand " << i << " after cast: " << *op << "\n";

      } else { // static dispatch
        operands.push_back(op);
      }
    }
    if (isDynamicDispatch) {
      // dynamic dispatch with vector return value
      // auto alloca = Builder.CreateAlloca(I->getType());
      // operands.push_back(alloca);

      // dynamic dispatch with pointer return value
      auto alloca = Builder.CreateAlloca(I->getType());
      // get type of last argument of F
      auto arg_type = F->getFunctionType()->getParamType(getArity(I));
      auto bitcast = Builder.CreateBitCast(alloca, arg_type);
      operands.push_back(bitcast);

      if (VfclibInstDebugOptions.isSet(debugOptions::GetOperands))
        errs() << "Result alloca: " << *alloca << "\n";
    }
    return operands;
  }

  typedef enum {
    x86_64_NONE = 0,
    x86_64_SSE = 1,
    x86_64_SSE2 = 2,
    x86_64_SSE3 = 3,
    x86_64_SSE4 = 4,
    x86_64_AVX = 5,
    x86_64_AVX2 = 6,
    x86_64_AVX512F = 7
  } X86_64_ISA;

  std::string getIsaAsStringX86_64(X86_64_ISA isa) {
    switch (isa) {
    case x86_64_NONE:
      return "";
    case x86_64_SSE:
      return "sse";
    case x86_64_SSE2:
      return "sse2";
    case x86_64_SSE3:
      return "sse3";
    case x86_64_SSE4:
      return "sse4";
    case x86_64_AVX:
      return "avx";
    case x86_64_AVX2:
      return "avx2";
    case x86_64_AVX512F:
      return "avx512f";
    default:
      return "";
    }
  }

  class TargetFeature {
  public:
    TargetFeature() = default;
    TargetFeature(const std::string &feature) {
      if (feature[0] == '-') {
        enabled = false;
        name = feature.substr(1);
      } else if (feature[0] == '+') {
        enabled = true;
        name = feature.substr(1);
      } else {
        enabled = true;
        name = feature;
      }
    }

    std::string getAsString() const {
      if (enabled) {
        return "+" + name;
      } else {
        return "-" + name;
      }
    }

    bool operator<(const TargetFeature &rhs) const {
      const auto repr = getAsString();
      const auto rhs_repr = rhs.getAsString();
      return repr < rhs_repr;
    }

    bool operator==(const TargetFeature &rhs) const {
      return (name == rhs.name) and (enabled == rhs.enabled);
    }

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const TargetFeature &TF) {
      OS << TF.getAsString();
      return OS;
    }

    bool enabled;
    std::string name;
  };

  class TargetFeatures {
  public:
    TargetFeatures() = default;
    TargetFeatures(const std::string &features) {
      if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
        errs() << "Build TargetFeatures from string: " << features << "\n";
      }
      std::vector<std::string> features_list = split(features, ',');
      if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
        errs() << "Features list: ";
        for (const auto &feature : features_list) {
          errs() << feature << " ";
        }
        errs() << "\n";
      }
      for (const auto &feature : features_list) {
        addAttribute(TargetFeature(feature));
      }
      if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
        errs() << "TargetFeatures built: " << getAsString() << "\n";
      }
    }

    TargetFeatures(const Attribute &features) {
      if (features.isStringAttribute()) {
        const auto features_str = features.getValueAsString();
        std::vector<std::string> features_list = split(features_str, ',');
        if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
          errs() << "Build TargetFeatures from Attribute: " << features_str
                 << "\n";
        }
        if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
          errs() << "Features list: ";
          for (const auto &feature : features_list) {
            errs() << feature << " ";
          }
          errs() << "\n";
        }
        for (const auto &feature : features_list) {
          addAttribute(TargetFeature(feature));
        }
      }
      if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
        errs() << "TargetFeatures built: " << getAsString() << "\n";
      }
    }

    void addAttribute(const std::string &feature) {
      if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
        errs() << "addAttribute string: " << feature << "\n";
      }
      if (this->hasAttribute(feature)) {
        if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
          errs() << "Attribute already present\n";
        }
        return;
      }
      addAttribute(TargetFeature(feature));
    }

    void addAttribute(const TargetFeature &feature) {
      if (VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures)) {
        errs() << "addAttribute TargetFeature: " << feature << "\n";
        if (features.find(feature) != features.end()) {
          errs() << "Attribute already present\n";
        }
      }
      features.insert(feature);
    }

    bool hasAttribute(const TargetFeature &feature) const {
      return features.find(feature) != features.end();
    }

    bool hasAttribute(const std::string &feature) const {
      return hasAttribute(TargetFeature(feature));
    }

    std::string getAsString() const {
      std::string str = "";
      for (const auto &feature : features) {
        str += feature.getAsString() + ",";
      }
      if (not str.empty()) {
        str.pop_back();
      }
      return str;
    }

    // Attribute getAttributes() {
    //   std::string str = getAsString();
    //   return Attribute::get(M.getContext(), str);
    // }

    bool isIncludeIn(const TargetFeatures &rhs) {
      for (const auto &feature : features) {
        if (not rhs.hasAttribute(feature)) {
          return false;
        }
      }
      return true;
    }

    bool empty() const { return features.empty(); }

    TargetFeatures operator+(const TargetFeatures &rhs) {
      TargetFeatures result;
      for (const auto &feature : features) {
        result.addAttribute(feature);
      }
      for (const auto &feature : rhs.features) {
        result.addAttribute(feature);
      }
      return result;
    }

    TargetFeatures operator-(const TargetFeatures &rhs) {
      TargetFeatures result;
      for (const auto &feature : features) {
        result.addAttribute(feature);
      }
      for (const auto &feature : rhs.features) {
        result.features.erase(feature);
      }
      return result;
    }

    TargetFeatures operator|(const TargetFeatures &rhs) {
      TargetFeatures result;
      for (const auto &feature : features) {
        result.addAttribute(feature);
      }
      for (const auto &feature : rhs.features) {
        result.addAttribute(feature);
      }
      return result;
    }

    TargetFeatures operator&(const TargetFeatures &rhs) {
      TargetFeatures result;
      for (const auto &feature : features) {
        if (rhs.hasAttribute(feature)) {
          result.addAttribute(feature);
        }
      }
      return result;
    }

    friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                         const TargetFeatures &TF) {
      OS << TF.getAsString();
      return OS;
    }

  private:
    std::set<TargetFeature> features;
  };

  X86_64_ISA getHighestSupportedISA_X86_64(const TargetFeatures &features) {
    if (features.hasAttribute("avx512f")) {
      return x86_64_AVX512F;
    }
    if (features.hasAttribute("avx2")) {
      return x86_64_AVX2;
    }
    if (features.hasAttribute("avx")) {
      return x86_64_AVX;
    }
    if (features.hasAttribute("sse4.2")) {
      return x86_64_SSE4;
    }
    if (features.hasAttribute("sse3")) {
      return x86_64_SSE3;
    }
    if (features.hasAttribute("sse2")) {
      return x86_64_SSE2;
    }
    if (features.hasAttribute("sse")) {
      return x86_64_SSE;
    }
    return x86_64_NONE;
  }

  bool hasFeatures_X86_64(const Attribute &src, const Attribute &target) {

    std::vector<std::string> features = {"avx512f", "avx2", "avx", "sse4.2",
                                         "sse3",    "sse2", "sse"};
    for (const auto &feature : features) {
      if (src.hasAttribute(feature) and not target.hasAttribute(feature)) {
        if (VfclibInstDebug) {
          errs() << "Missing feature: " << feature << "\n";
        }
        return false;
      }
    }
    return true;
  }

  bool areABICompatible(Instruction *CallInst, Function *Callee) {
    auto caller = CallInst->getParent()->getParent();
    auto calleeTargetFeatures =
        TargetFeatures(Callee->getFnAttribute("target-features"));
    auto callerTargetFeatures =
        TargetFeatures(caller->getFnAttribute("target-features"));

    if (callerTargetFeatures.empty()) {
      // The case when using flang as frontend
      // TODO: add a flag to enable/disable this
      return true;
    }

    // if caller does have the callee features, we're good
    if (calleeTargetFeatures.isIncludeIn(callerTargetFeatures)) {
      if (VfclibInstDebug and VfclibInstDebugOptions.isSet(debugOptions::ABI)) {
        errs() << "Caller has the callee target features\n";
        errs() << "caller features: " << callerTargetFeatures.getAsString()
               << "\n";
        errs() << "callee features: " << calleeTargetFeatures.getAsString()
               << "\n";
      }
      return true;
    } else {
      // if caller does not have the callee features, we need to check if there
      // is compatibility
      auto caller_highest_isa_supported =
          getHighestSupportedISA_X86_64(callerTargetFeatures);
      auto callee_highest_isa_supported =
          getHighestSupportedISA_X86_64(calleeTargetFeatures);
      if (caller_highest_isa_supported >= callee_highest_isa_supported) {
        if (VfclibInstDebug and
            VfclibInstDebugOptions.isSet(debugOptions::ABI)) {
          errs() << "Caller is compatible with callee target features\n";
          errs() << "caller features: " << callerTargetFeatures.getAsString()
                 << "\n";
          errs() << "callee features: " << calleeTargetFeatures.getAsString()
                 << "\n";
        }
        return true;
      } else {
        if (VfclibInstDebug and
            VfclibInstDebugOptions.isSet(debugOptions::ABI)) {
          errs() << "Caller is not compatible with callee target features\n";
          errs() << "caller features: " << callerTargetFeatures.getAsString()
                 << "\n";
          errs() << "callee features: " << calleeTargetFeatures.getAsString()
                 << "\n";
        }
        return false;
      }
    }
  }

  /* Replace arithmetic instructions with PR */
  Value *replaceArithmeticWithPRCall(IRBuilder<> &Builder, Instruction *I) {
    Function *F = getPRFunction(I);
    if (F == nullptr) {
      return nullptr;
    }

    if (not areABICompatible(I, F))
      return nullptr;

    const auto dynamic_dispatch = is_dynamic_dispatch(F);
    auto operands = getOperands(Builder, I, F, dynamic_dispatch);

    auto call = Builder.CreateCall(F, operands);
    call->setAttributes(F->getAttributes());

    Value *result = nullptr;

    if (dynamic_dispatch) {
      // dynamic dispatch with vector return value
      // auto load = Builder.CreateLoad(I->getType(), operands.back());
      // result = load;

      // dynamic dispatch with pointer return value
      auto return_type = I->getType();
      auto return_type_ptr = return_type->getPointerTo();
      auto ptr_to_vec = Builder.CreateBitCast(operands.back(), return_type_ptr);
      auto load = Builder.CreateLoad(return_type, ptr_to_vec);
      result = load;

    } else {
      auto return_type = F->getReturnType();
      if (isf32x2Value(I) and return_type->isDoubleTy()) {
        auto bitcast = Builder.CreateBitCast(call, I->getType());
        result = bitcast;
      } else {
        result = call;
      }
    }

    return result;
  }

  Value *replaceWithPRCall(Module &M, Instruction *I, Fops opCode) {
    if (not isValidInstruction(I)) {
      return nullptr;
    }

    IRBuilder<> Builder(I);

    // Check if this instruction is the last one in the block
    if (I->isTerminator()) {
      // Insert at the end of the block
      Builder.SetInsertPoint(I->getParent());
    } else {
      // Set insertion point after the given instruction
      Builder.SetInsertPoint(I->getNextNode());
    }

    Value *newInst = replaceArithmeticWithPRCall(Builder, I);

    return newInst;
  }

  bool isFMAOperation(const Instruction &I) {
    auto CI = static_cast<const CallInst *>(&I);
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

  Fops getOpCode(const Instruction &I) {
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
      return (VfclibInstInstrumentFMA and isFMAOperation(I)) ? FOP_FMA
                                                             : FOP_IGNORE;
    case Instruction::FCmp:
      return FOP_IGNORE;
    default:
      return FOP_IGNORE;
    }
  }

  Fops mustReplace(Instruction &I) { return getOpCode(I); }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {
    bool modified = false;
    std::set<std::pair<Instruction *, Fops>> WorkList;
    for (auto &I : B) {
      auto opCode = mustReplace(I);
      if (opCode == FOP_IGNORE)
        continue;
      WorkList.insert(std::make_pair(&I, opCode));
    }

    for (auto p : WorkList) {
      Instruction *I = p.first;
      Fops opCode = p.second;
      Value *value = replaceWithPRCall(M, I, opCode);
      if (value != nullptr) {
        if (VfclibInstVerbose)
          errs() << "Instrumenting" << *I << '\n';
        BasicBlock::iterator ii(I);
#if LLVM_VERSION_MAJOR >= 16
        ReplaceInstWithValue(ii, value);
#else
        ReplaceInstWithValue(B.getInstList(), ii, value);
#endif
      }
    }

    return modified;
  }
}; // namespace
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass",
                                  false, false);
