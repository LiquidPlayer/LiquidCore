//
//  Module.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "V82JSC.h"

using namespace v8;

/**
 * Returns the module's current status.
 */
Module::Status Module::GetStatus() const
{
    assert(0);
}

/**
 * For a module in kErrored status, this returns the corresponding exception.
 */
Local<Value> Module::GetException() const
{
    assert(0);
}

/**
 * Returns the number of modules requested by this module.
 */
int Module::GetModuleRequestsLength() const
{
    assert(0);
}

/**
 * Returns the ith module specifier in this module.
 * i must be < GetModuleRequestsLength() and >= 0.
 */
Local<String> Module::GetModuleRequest(int i) const
{
    assert(0);
}

/**
 * Returns the source location (line number and column number) of the ith
 * module specifier's first occurrence in this module.
 */
Location Module::GetModuleRequestLocation(int i) const
{
    assert(0);
}

/**
 * Returns the identity hash for this object.
 */
int Module::GetIdentityHash() const
{
    assert(0);
}

/**
 * ModuleDeclarationInstantiation
 *
 * Returns an empty Maybe<bool> if an exception occurred during
 * instantiation. (In the case where the callback throws an exception, that
 * exception is propagated.)
 */
Maybe<bool> Module::InstantiateModule(Local<Context> context, ResolveCallback callback)
{
    assert(0);
}

/**
 * ModuleEvaluation
 *
 * Returns the completion value.
 * TODO(neis): Be more precise or say nothing.
 */
MaybeLocal<Value> Module::Evaluate(Local<Context> context)
{
    assert(0);
}

/**
 * Returns the namespace object of this module. The module must have
 * been successfully instantiated before and must not be errored.
 */
Local<Value> Module::GetModuleNamespace()
{
    assert(0);
}

