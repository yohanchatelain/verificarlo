/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015-2020                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

#include "../../../config.h"
#include "../libVfcInstrumentCommon.hxx"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <fstream>
#include <set>
#include <utility>

#include "../../common/interflop.h"

#include "libVfcTracerHandler/vfcTracerHandler.hxx"

using namespace llvm;

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static const std::string external_ch_name = "interflop_call";
static const std::string internal_ch_name = "_handle_call";

namespace {

struct VfclibHandlerInst : public ModulePass {
  static char ID;

  VfclibHandlerInst() : ModulePass(ID) {}

  void exitHandlers() { interflop::tracer::exit(); }

  bool runOnModule(Module &M) {
    bool modified = false;
    for (auto &F : M) {
      modified |= runOnFunction(M, F);
    }
    exitHandlers();
    return modified;
  }

  bool runOnFunction(Module &M, Function &F) {
    bool modified = false;
    for (auto &BI : F) {
      modified |= runOnBasicBlock(M, BI);
    }
    return modified;
  }

  Value *replaceCall(Module &M, CallInst *CI) {
    IRBuilder<> Builder(CI);

    // void *interflop_handle_call(int destination, INTERFLOP_CALL_OPCODE
    // opcode, ...);

    Type *arg0Ty = Builder.getInt32Ty();
    Type *arg1Ty = Builder.getInt32Ty();

    unsigned nb_args = CI->arg_size();
    Value *arg0 = CI->getArgOperand(0);
    Value *arg1 = CI->getArgOperand(1);
    std::vector<Value *> args_vec = {arg0, arg1};
    for (int i = 2; i < nb_args; i++) {
      args_vec.push_back(CI->getArgOperand(i));
    }
    ArrayRef<Value *> args(args_vec);

    Type *retType = Builder.getInt8PtrTy();
    FunctionType *funType = FunctionType::get(retType, {arg0Ty, arg1Ty}, true);
    _LLVMFunctionType internalHandleCall =
        M.getOrInsertFunction(internal_ch_name, funType);

    Value *newInst = Builder.CreateCall(internalHandleCall, args);
    return newInst;
  }

  Value *replaceWithHandler(Module &M, CallInst *CI) {
    Value *callOpcodeValue = CI->getArgOperand(1);
    if (ConstantInt *cst = dyn_cast<ConstantInt>(callOpcodeValue)) {
      uint64_t value = cst->getZExtValue();
      switch (value) {
      default:
        return replaceCall(M, CI);
      }
    }
    return nullptr;
  }

  bool mustHandle(CallInst &CI) {
    Function *calledFunction = CI.getCalledFunction();
    if (calledFunction) {
      if (calledFunction->getName() == external_ch_name) {
        Value *destination = CI.getArgOperand(0);
        if (ConstantInt *cst = dyn_cast<ConstantInt>(destination)) {
          int64_t value = cst->getSExtValue();
          if (value < 1) {
            return true;
          } else {
            return false;
          }
        }
      }
    }
    return false;
  }

  bool mustReplace(Instruction &I) {
    if (CallInst *CI = dyn_cast<CallInst>(&I)) {
      return mustHandle(*CI);
    } else {
      return false;
    }
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {
    bool modified = false;
    std::vector<Instruction *> workList;
    for (auto &I : B) {
      if (mustReplace(I))
        workList.push_back(&I);
    }

    for (auto &I : workList) {
      CallInst *CI = cast<CallInst>(I);
      if (VfclibInstVerbose)
        errs() << "Handling" << *CI << "\n";
      Value *value = replaceWithHandler(M, CI);
      if (value != nullptr) {
        BasicBlock::iterator ii(CI);
        ReplaceInstWithValue(B.getInstList(), ii, value);
      }
    }

    return modified;
  }
};
} // namespace

char VfclibHandlerInst::ID = 0;
static RegisterPass<VfclibHandlerInst>
    X("vfclibhandlerinst", "verificarlo handler instrument pass", false, false);
