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

#ifndef FORMAT_FORMAT_HXX
#define FORMAT_FORMAT_HXX

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include <string>

#include "../Data/Data.hxx"

#if LLVM_VERSION_MAJOR < 5
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__, (Type *)NULL)
typedef llvm::Constant *_LLVMFunctionType;
#elif LLVM_VERSION_MAJOR < 9
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__)
typedef llvm::Constant *_LLVMFunctionType;
#else
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__)
typedef llvm::FunctionCallee _LLVMFunctionType;
#endif
namespace vfctracerFormat {

enum FormatId { BinaryId, TextId };

class Format {
  FormatId Id;

protected:
  llvm::Module *M;
  llvm::Type *locInfoType;
  llvm::Value *locInfoValue;

public:
  FormatId getValueId() const { return Id; };
  Format(FormatId id)
      : Id(id), M(nullptr), locInfoType(nullptr), locInfoValue(nullptr) {}
  virtual llvm::Type *getLocInfoType(vfctracerData::Data &D) = 0;
  virtual llvm::Type *getLocInfoType(vfctracerData::Data *D) = 0;
  virtual llvm::Value *getOrCreateLocInfoValue(vfctracerData::Data &D) = 0;
  virtual llvm::Value *getOrCreateLocInfoValue(vfctracerData::Data *D) = 0;
  virtual _LLVMFunctionType
  CreateProbeFunctionPrototype(vfctracerData::Data &D) = 0;
  virtual llvm::CallInst *
  InsertProbeFunctionCall(vfctracerData::Data &D,
                          _LLVMFunctionType probeFunc) = 0;
};

class BinaryFmt : public Format {
public:
  BinaryFmt(llvm::Module &M) : Format(BinaryId) { this->M = &M; };
  llvm::Type *getLocInfoType(vfctracerData::Data &D);
  llvm::Type *getLocInfoType(vfctracerData::Data *D);
  llvm::Value *getOrCreateLocInfoValue(vfctracerData::Data &D);
  llvm::Value *getOrCreateLocInfoValue(vfctracerData::Data *D);
  _LLVMFunctionType CreateProbeFunctionPrototype(vfctracerData::Data &D);
  llvm::CallInst *InsertProbeFunctionCall(vfctracerData::Data &D,
                                          _LLVMFunctionType probeFunc);
};

class TextFmt : public Format {
public:
  TextFmt(llvm::Module &M) : Format(TextId) { this->M = &M; };
  llvm::Type *getLocInfoType(vfctracerData::Data &D);
  llvm::Type *getLocInfoType(vfctracerData::Data *D);
  llvm::Value *getOrCreateLocInfoValue(vfctracerData::Data &D);
  llvm::Value *getOrCreateLocInfoValue(vfctracerData::Data *D);
  _LLVMFunctionType CreateProbeFunctionPrototype(vfctracerData::Data &D);
  llvm::CallInst *InsertProbeFunctionCall(vfctracerData::Data &D,
                                          _LLVMFunctionType probeFunc);
};

Format *CreateFormat(llvm::Module &M, vfctracerFormat::FormatId optFmt);
} // namespace vfctracerFormat

#endif /* FORMAT_FORMAT_HXX */
