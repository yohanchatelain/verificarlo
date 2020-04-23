/******************************************************************************
 *                                                                            *
 *  This file is part of Verificarlo.                                         *
 *                                                                            *
 *  Copyright (c) 2015                                                        *
 *     Universite de Versailles St-Quentin-en-Yvelines                        *
 *     CMLA, Ecole Normale Superieure de Cachan                               *
 *  Copyright (c) 2018                                                        *
 *     Universite de Versailles St-Quentin-en-Yvelines                        *
 *  Copyright (c) 2019-2020                                                   *
 *     Verificarlo contributors                                               *
 *                                                                            *
 *  Verificarlo is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation, either version 3 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  Verificarlo is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.      *
 *                                                                            *
 ******************************************************************************/

#include "../../config.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Function.h"

#include <fstream>
#include <set>
#include <utility>

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

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<bool>
    VfclibInstInstrumentFCMP("vfclibinst-inst-fcmp",
                             cl::desc("Instrument floating point comparisons"),
                             cl::value_desc("InstrumentFCMP"), cl::init(false));

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo

enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_CMP, FOP_IGNORE };

  // Each instruction can be translated to a string representation

  std::string Fops2str[] = {"add", "sub", "mul", "div", "cmp", "ignore"};

  typedef std::map<Fops, int> mapFops;
  typedef std::map<Function*, mapFops> mapF;
  
struct VfclibReport : public ModulePass {
  static char ID;

  std::set<std::string> IncludedFunctionSet;
  std::set<std::string> ExcludedFunctionSet;
  
  const std::string reportPathEnv = "VERIFICARLO_REPORT_PATH";
  const std::string reportFilenameIncluded = "report_included.csv";
  const std::string reportFilenameExcluded = "report_excluded.csv";
  std::ofstream reportFileIncluded, reportFileExcluded;

  mapF mapIncludedFunctions;
  mapF mapExcludedFunctions;

  VfclibReport() : ModulePass(ID) {
    openReportFile(reportFileIncluded, reportFilenameIncluded);
    openReportFile(reportFileExcluded, reportFilenameExcluded);
  }

  void parseFunctionSetFile(Module &M, cl::opt<std::string> &fileName,
                            std::set<std::string> &FunctionSet) {
    // Skip if empty fileName
    if (fileName.empty()) {
      return;
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
    // drop the .1.ll suffix in the module name
    StringRef mod_name = StringRef(M.getModuleIdentifier()).drop_back(5);
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
        if (p.first.trim().equals(mod_name) || p.first.trim().equals("*")) {
          FunctionSet.insert(p.second.trim());
        }
      }
    }

    loopstream.close();    
  }

  void openReportFile(std::ofstream &reportFile, const std::string &reportFilename) {
    char* env_c_str = std::getenv(reportPathEnv.c_str());
    std::string reportFilenamePath;
    if (env_c_str == NULL) {
      reportFilenamePath = reportFilename;
    } else {
      reportFilenamePath = std::string(env_c_str) + reportFilename;
    }
    reportFile.open(reportFilenamePath, std::fstream::out | std::fstream::app);
    if (not reportFile.is_open()) {
      report_fatal_error("Cannot open report file : " + reportFilenamePath);      
    }
  }

  std::string getLinkageTypeStr(Function &F) {
    GlobalValue::LinkageTypes linkageType = F.getLinkage();
    switch (linkageType) {
    case GlobalValue::ExternalLinkage:
      return "ExternalLinkage";
    case GlobalValue::AvailableExternallyLinkage:
      return "AvailableExternallyLinkage";
    case GlobalValue::LinkOnceAnyLinkage:
      return "LinkOnceAnyLinkage";
    case GlobalValue::LinkOnceODRLinkage:
      return "LinkOnceODRLinkage";
    case GlobalValue::WeakAnyLinkage:
      return "WeakAnyLinkage";
    case GlobalValue::WeakODRLinkage:
      return "WeakODRLinkage";
    case GlobalValue::AppendingLinkage:
      return "AppendingLinkage";
    case GlobalValue::InternalLinkage:
      return "InternalLinkage";
    case GlobalValue::PrivateLinkage:
      return "PrivateLinkage";
    case GlobalValue::ExternalWeakLinkage:
      return "ExternalWeakLinkage";
    case GlobalValue::CommonLinkage:
      return "CommonLinkage";
    default:
      report_fatal_error("Unknown linkage");
    }
  }

  bool isDeclaration(Function &F) {
    const Function::BasicBlockListType &bb_list = F.getBasicBlockList();
    if (bb_list.empty()) {
      return true;
    } else {
      return false;
    }
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    // Parse both included and excluded function set
    parseFunctionSetFile(M, VfclibInstIncludeFile, IncludedFunctionSet);
    parseFunctionSetFile(M, VfclibInstExcludeFile, ExcludedFunctionSet);

    // Parse instrument single function option (--function)
    if (!VfclibInstFunction.empty()) {
      IncludedFunctionSet.insert(VfclibInstFunction);
      ExcludedFunctionSet.insert("*");
    }

    // Find the list of functions to instrument
    std::vector<Function *> includedFunctions;
    std::vector<Function *> excludedFunctions;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {

      if (isDeclaration(*F))
	continue;
      
      // White-list
      if (IncludedFunctionSet.find("*") != IncludedFunctionSet.end() ||
          IncludedFunctionSet.find(F->getName()) != IncludedFunctionSet.end()) {
        includedFunctions.push_back(&*F);
        continue;
      }

      // Black-list
      if (ExcludedFunctionSet.find("*") != ExcludedFunctionSet.end() ||
          ExcludedFunctionSet.find(F->getName()) != ExcludedFunctionSet.end()) {
	excludedFunctions.push_back(&*F);
        continue;
      }

      // If black-list is empty and while-list is not, we are done
      if (VfclibInstExcludeFile.empty() && !VfclibInstIncludeFile.empty()) {
        continue;
      } else {
        // Everything else is neither white-listed or black-listed
        includedFunctions.push_back(&*F);
      }
    }
    
    // Report FP instructions on functions to instrument
    for (auto F : includedFunctions) {
      modified |= runOnFunction(*F, true);
    }
    // Report FP instructions on functions to not instrument
    for (auto F : excludedFunctions) {
      modified |= runOnFunction(*F, false);     
    }

    errs() << "### Report ###\n";

    if (not includedFunctions.empty()) {
      printMapF(M, reportFileIncluded, mapIncludedFunctions);
    }
    if (not excludedFunctions.empty()) {
      printMapF(M, reportFileExcluded, mapExcludedFunctions);
    }

    const int nbIncludeFun = VfclibInstIncludeFile.size();
    const int nbExcludeFun = VfclibInstExcludeFile.size();
    const int nbTotalFun = nbIncludeFun + nbExcludeFun;
    
    const int nbIncludeOps = countOpsInMapf(mapIncludedFunctions);
    const int nbExcludeOps = countOpsInMapf(mapExcludedFunctions);
    const int nbTotalOps = nbIncludeOps + nbExcludeOps;
    
    errs() << "Functions:\n";
    errs() << "\tTotal: " << std::to_string(nbTotalFun) << "\n";
    errs() << "\tInstrumented: " << std::to_string(nbIncludeFun) << "\n";
    errs() << "\tNot Instrumented: " << std::to_string(nbExcludeFun) << "\n";

    errs() << "Floating-point operations:\n";
    errs() << "\tTotal: " << std::to_string(nbTotalOps) << "\n";
    errs() << "\tInstrumented: " << std::to_string(nbIncludeOps) << "\n";
    errs() << "\tNot Instrumented: " << std::to_string(nbExcludeOps) << "\n";
    
    // runOnModule must return true if the pass modifies the IR
    return modified;
  }

  int countOpsInMapf(mapF map) {
    int total = 0;
    // pair <Function, mapOps>
    for (auto p1 : map) {
      for (auto p2 : p1.second) {
	total += p2.second;
      }
    }
    return total;
  }

  void printMapF(Module &M, std::ofstream &reportFile, mapF map) {

    const std::string moduleName = M.getName().str();
    for (auto p1 : map) {
      int total = 0;
      Function *F = p1.first;
      const std::string functionName = F->getName().str();
      reportFile << moduleName << ";";
      reportFile << functionName << ";";
      for (auto p2 : p1.second) {
	total += p2.second;
	reportFile << p2.second << ";";
      }
      reportFile << total << "\n";
    }
  }
  
  bool runOnFunction(Function &F, bool toInstrument) {
    mapFops *mapFunction = (toInstrument) ? &mapIncludedFunctions[&F] : &mapExcludedFunctions[&F];   
    *mapFunction = {{FOP_ADD,0}, {FOP_SUB,0}, {FOP_MUL,0}, {FOP_DIV,0}, {FOP_CMP,0}};
    
    for (auto &BB : F) {      
      runOnBasicBlock(BB, *mapFunction);
    }

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
    case Instruction::FCmp:
      // Only instrument FCMP if the flag --inst-fcmp is passed
      if (VfclibInstInstrumentFCMP) {
        return FOP_CMP;
      } else {
        return FOP_IGNORE;
      }
    default:
      return FOP_IGNORE;
    }
  }

  bool runOnBasicBlock(BasicBlock &BB, std::map<Fops,int> &fopsCounter) {
    for (auto &I : BB) {
      Fops opCode = mustReplace(I);
      if (opCode == FOP_IGNORE) {
        continue;
      } else {
	fopsCounter[opCode]++;
      }
    }

    return false;
  }
};
} // namespace

char VfclibReport::ID = 0;
static RegisterPass<VfclibReport> X("vfclibreport", "verificarlo reporting pass",
                                  false, false);
