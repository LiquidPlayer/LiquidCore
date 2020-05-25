/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

/**
 * Returns the module's current status.
 */
Module::Status Module::GetStatus() const
{
    NOT_IMPLEMENTED;
}

/**
 * For a module in kErrored status, this returns the corresponding exception.
 */
Local<v8::Value> Module::GetException() const
{
    NOT_IMPLEMENTED;
}

/**
 * Returns the number of modules requested by this module.
 */
int Module::GetModuleRequestsLength() const
{
    NOT_IMPLEMENTED;
}

/**
 * Returns the ith module specifier in this module.
 * i must be < GetModuleRequestsLength() and >= 0.
 */
Local<String> Module::GetModuleRequest(int i) const
{
    NOT_IMPLEMENTED;
}

/**
 * Returns the source location (line number and column number) of the ith
 * module specifier's first occurrence in this module.
 */
Location Module::GetModuleRequestLocation(int i) const
{
    NOT_IMPLEMENTED;
}

/**
 * Returns the identity hash for this object.
 */
int Module::GetIdentityHash() const
{
    NOT_IMPLEMENTED;
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
    NOT_IMPLEMENTED;
}

/**
 * ModuleEvaluation
 *
 * Returns the completion value.
 * TODO(neis): Be more precise or say nothing.
 */
MaybeLocal<v8::Value> Module::Evaluate(Local<Context> context)
{
    NOT_IMPLEMENTED;
}

/**
 * Returns the namespace object of this module. The module must have
 * been successfully instantiated before and must not be errored.
 */
Local<v8::Value> Module::GetModuleNamespace()
{
    NOT_IMPLEMENTED;
}

