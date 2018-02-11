//
//  Function.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Function.h"
#include "Utils.h"

using namespace v8;

/**
 * Create a function in the current execution context
 * for a given FunctionCallback.
 */
MaybeLocal<Function> Function::New(Local<Context> context, FunctionCallback callback,
                                Local<Value> data, int length,
                                ConstructorBehavior behavior)
{
    return MaybeLocal<Function>();
}


MaybeLocal<Object> Function::NewInstance(Local<Context> context, int argc, Local<Value> argv[]) const
{
    return MaybeLocal<Object>();
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> recv, int argc,
                                 Local<Value> argv[])
{
    return MaybeLocal<Value>();
}

void Function::SetName(Local<String> name)
{
    
}

Local<Value> Function::GetName() const
{
    return Utils::NewValue(nullptr);
}

/**
 * Name inferred from variable or property assignment of this function.
 * Used to facilitate debugging and profiling of JavaScript code written
 * in an OO style, where many functions are anonymous but are assigned
 * to object properties.
 */
Local<Value> Function::GetInferredName() const
{
    return Utils::NewValue(nullptr);
}

/**
 * displayName if it is set, otherwise name if it is configured, otherwise
 * function name, otherwise inferred name.
 */
Local<Value> Function::GetDebugName() const
{
    return Utils::NewValue(nullptr);
}

/**
 * User-defined name assigned to the "displayName" property of this function.
 * Used to facilitate debugging and profiling of JavaScript code.
 */
Local<Value> Function::GetDisplayName() const
{
    return Utils::NewValue(nullptr);
}

/**
 * Returns zero based line number of function body and
 * kLineOffsetNotFound if no information available.
 */
int Function::GetScriptLineNumber() const
{
    return 0;
}
/**
 * Returns zero based column number of function body and
 * kLineOffsetNotFound if no information available.
 */
int Function::GetScriptColumnNumber() const
{
    return 0;
}

/**
 * Returns scriptId.
 */
int Function::ScriptId() const
{
    return 0;
}

/**
 * Returns the original function if this function is bound, else returns
 * v8::Undefined.
 */
Local<Value> Function::GetBoundFunction() const
{
    return Utils::NewValue(nullptr);
}

ScriptOrigin Function::GetScriptOrigin() const
{
    ScriptOrigin so(Utils::NewValue(nullptr));
    return so;
}
