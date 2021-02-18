/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo. *
 *                                                                              *
 *  Copyright (c) 2018 * Universite de Versailles St-Quentin-en-Yvelines * CMLA,
 *Ecole Normale Superieure de Cachan                                 *
 *                                                                              *
 *  Verificarlo is free software: you can redistribute it and/or modify * it
 *under the terms of the GNU General Public License as published by        * the
 *Free Software Foundation, either version 3 of the License, or           * (at
 *your option) any later version.                                         *
 *                                                                              *
 *  Verificarlo is distributed in the hope that it will be useful, * but WITHOUT
 *ANY WARRANTY; without even the implied warranty of              *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the * GNU General
 *Public License for more details.                                *
 *                                                                              *
 *  You should have received a copy of the GNU General Public License * along
 *with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.        *
 *                                                                              *
 ********************************************************************************/

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
// #include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <sstream>
#include <string>

#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <unordered_map>

#include "../opcode.hxx"
#include "../vfctracer.hxx"
#include "Data.hxx"

namespace vfctracerData {

using namespace llvm;

Data::Data(Instruction *I, DataId id) : Id(id) {
  data = I;
  dataName = "";
  dataRawName = "";
  originalLine = "";
  baseTypeName = "";
  BB = I->getParent();
  F = BB->getParent();
  M = F->getParent();
  operationCode = opcode::getOpCode(data);
  switch (operationCode) {
  case opcode::Fops::STORE:
    baseType = data->getOperand(0)->getType();
    basePointerType = data->getOperand(1)->getType();
    break;
  case opcode::Fops::RETURN:
    if (data->getNumOperands() != 0) {
      baseType = data->getOperand(0)->getType();
      basePointerType = baseType->getPointerTo();
    } else { /* data = ret void */
      baseType = data->getType();
      basePointerType = nullptr;
    }
    break;
  case opcode::Fops::CALLINST: {
    llvm::CallInst *callInst = dyn_cast<llvm::CallInst>(I);
    baseType = callInst->getArgOperand(0)->getType();
    basePointerType = callInst->getArgOperand(0)->getType();
  } break;
  default:
    baseType = data->getType();
    basePointerType = baseType->getPointerTo();
  }
}

Module *Data::getModule() { return M; }

void Data::dump() {
  if (isa<vfctracerData::ScalarData>(this))
    errs() << "[ScalarData]\n";
  else if (isa<vfctracerData::VectorData>(this))
    errs() << "[VectorData]\n";
  else if (isa<vfctracerData::ProbeData>(this))
    errs() << "[ProbeData]\n";

  errs() << "Data: " << *getData() << "\n"
         << "OpCode: " << opcode::fops_str(opcode::getOpCode(data)) << "\n"
         << "RawName: " << getRawName() << "\n"
         << "Value: " << *getValue() << "\n"
         << "Type: " << *getDataType() << "\n"
         << "Line: " << getOriginalLine() << "\n"
         << "Function: " << getFunctionName() << "\n"
         << "VariableName: " << getVariableName() << "\n";
  std::string sep(80, '-');
  errs() << sep << "\n";
}

bool Data::isTemporaryVariable() const {
  return this->dataName.empty() ||
         this->dataName == vfctracer::temporaryVariableName;
}

std::string Data::getOriginalLine() {
  if (not originalLine.empty())
    return originalLine;

  originalLine = vfctracer::temporaryVariableName;

  Instruction *data = getData();

  if (opcode::isStoreOp(data)) {
    MDNode *N = data->getMetadata("dbg");
    MDNode *N1 = vfctracer::findVar(data, F);
    MDNode *N2 = vfctracer::findVar(data->getOperand(1), F);
    if (not N)
      N = N1 == nullptr ? N2 : N1;

    if (N) {
      unsigned line = 0, column = 0;
      std::string File;
/* Try to get information about the address variable */
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 7
      DIVariable Loc(N);
      line = Loc.getLineNumber();
      Loc.getFile();
#else
      if (DILocalVariable *DILocVar = dyn_cast<DILocalVariable>(N)) {
        line = DILocVar->getLine();
        File = DILocVar->getFilename();
      }
      if (DILocation *DILocVar = dyn_cast<DILocation>(N)) {
        line = DILocVar->getLine();
        column = DILocVar->getColumn();
        File = DILocVar->getFilename();
      }

#endif
      originalLine = File;
      originalLine += " " + std::to_string(line) + "." + std::to_string(column);
    }
  } else {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 7
    if (MDNode *N = data->getMetadata(LLVMContext::MD_dbg)) {
      DILocation Loc(N);
      std::string Line = std::to_string(Loc.getLineNumber());
      std::string Column = std::to_string(Loc.getColumnNumber());
      std::string File = Loc.getFilename();
      std::string Dir = Loc.getDirectory();
      originalLine = File + " " + Line + "." + Column;
    }
#else
    DebugLoc Loc = data->getDebugLoc();
    if (not Loc)
      return originalLine;
    unsigned line = Loc->getLine();
    std::string Line = std::to_string(line);
    unsigned column = Loc->getColumn();
    std::string Column = std::to_string(column);
    std::string File = Loc->getFilename();
    std::string Dir = Loc->getDirectory();
    originalLine = File + " " + Line + "." + Column;

#endif
  }
  return originalLine;
}

Instruction *Data::getData() const { return data; }

Value *Data::getValue() const {
  if (opcode::isFPOp(data))
    return data;
  else if (opcode::isStoreOp(data))
    return data->getOperand(0);
  else if (opcode::isRetOp(data))
    return data->getOperand(0);
  else
    llvm_unreachable("Operation unknown");
}

Type *Data::getDataType() const { return baseType; }

Type *Data::getDataPtrType() const { return basePointerType; }

std::string Data::getFunctionName() { return F->getName().str(); }

std::string &Data::getRawName() {
  if (not dataRawName.empty())
    return dataRawName;
  dataRawName = vfctracer::getRawName(data);
  return dataRawName;
}

bool Data::isValidOperation() const {
  Instruction *I = getData();
  /* Checks that the stored value is not a constant */
  if (opcode::isStoreOp(I))
    return not isa<llvm::ConstantFP>(getValue());
  /* Checks that the returned value type is not void */
  if (opcode::isRetOp(I))
    return not getDataType()->isVoidTy();
  return not opcode::isIgnoreOp(I);
}

bool Data::isValidDataType() const {
  if (baseType->isFloatTy())
    return true;
  else if (baseType->isDoubleTy())
    return true;
  else if (llvm::PointerType *PtrTy = dyn_cast<llvm::PointerType>(baseType)) {
    if (PtrTy->getElementType()->isFloatTy())
      return true;
    else if (PtrTy->getElementType()->isDoubleTy())
      return true;
    else
      return false;
  } else
    return false;
}

/* Smart constructor */
Data *CreateData(Instruction *I) {
  /* Checks if instruction is well formed */
  if (I->getParent() == nullptr)
    return nullptr; /* Instruction is not currently inserted into a BasicBlock
                     */
  if (I->getParent()->getParent() == nullptr)
    return nullptr; /* Instruction is not currently inserted into a function*/

  if (opcode::isRetOp(I) and I->getType()->isVoidTy()) {
    return nullptr;
  }

  /* Avoid returning call instruction other than vfc_probe */
  if (opcode::isCallOp(I) && not opcode::isProbeOp(I))
    return nullptr;

  if (opcode::isVectorOp(I))
    return new vfctracerData::VectorData(I);
  else if (opcode::isProbeOp(I))
    return new vfctracerData::ProbeData(I);
  else
    return new vfctracerData::ScalarData(I);
}
} // namespace vfctracerData
