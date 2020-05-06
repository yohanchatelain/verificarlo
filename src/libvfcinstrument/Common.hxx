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

#ifndef
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall3(func, op1, op2, op3, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall2(func, op1, op2, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(i, p))
/* This function must be used with at least one variadic argument otherwise */
/* it will fails when compiling since it will expand as
 * M.getOrInsertFunction(name,res,,(Type*)NULL) */
/* It could be fixed when __VA_OPT__ will be available (see
 * https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html)*/
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__, (Type *)NULL)
typedef llvm::Constant *_LLVMFunctionType;
#elif LLVM_VERSION_MAJOR < 5
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
