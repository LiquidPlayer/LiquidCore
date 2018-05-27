//
//  Function.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/**
 * Create a function in the current execution context
 * for a given FunctionCallback.
 */
MaybeLocal<Function> Function::New(Local<Context> context, FunctionCallback callback,
                                Local<Value> data, int length,
                                ConstructorBehavior behavior)
{
    assert(0);
    return MaybeLocal<Function>();
}


MaybeLocal<Object> Function::NewInstance(Local<Context> context, int argc, Local<Value> argv[]) const
{
    JSObjectRef func = (JSObjectRef) V82JSC::ToJSValueRef<Function>(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));
    JSValueRef args[argc];
    for (int i=0; i<argc; i++) {
        args[i] = V82JSC::ToJSValueRef<Value>(argv[i], context);
    }
    LocalException exception(iso);
    JSValueRef excp = 0;
    InstanceWrap *wrap = V82JSC::getPrivateInstance(V82JSC::ToContextRef(context), func);
    JSGlobalContextRef creationContext = wrap ? wrap->m_creation_context : JSContextGetGlobalContext(V82JSC::ToContextRef(context));
    
    JSObjectRef newobj = JSObjectCallAsConstructor(V82JSC::ToContextRef(context), func, argc, args, &excp);
    if (!newobj && !excp) {
        V82JSC::exec(V82JSC::ToContextRef(context), "throw new TypeError(_1.name + ' is not a constructor');", 1, &func, &exception);
    } else {
        exception.exception_ = excp;
    }
    if (!exception.ShouldThow()) {
        if (wrap) {
            // A new instance from a function created from a template will always have the creation context
            // of the original function
            wrap = V82JSC::makePrivateInstance(iso, V82JSC::ToContextRef(context), newobj);
            if (wrap) wrap->m_creation_context = creationContext;
        }
        return ValueImpl::New(V82JSC::ToContextImpl(context), newobj).As<Object>();
    }
    return MaybeLocal<Object>();
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> recv, int argc,
                                 Local<Value> argv[])
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSObjectRef func = (JSObjectRef) V82JSC::ToJSValueRef<Function>(this, context);
    JSValueRef thiz = V82JSC::ToJSValueRef<Value>(recv, context);
    JSValueRef args[argc];
    for (int i=0; i<argc; i++) {
        args[i] = V82JSC::ToJSValueRef<Value>(argv[i], context);
    }
    LocalException exception(iso);
    JSValueRef excp = 0;
    JSValueRef result = JSObjectCallAsFunction(V82JSC::ToContextRef(context), func, (JSObjectRef)thiz, argc, args, &excp);
    if (!result && !excp) {
        V82JSC::exec(V82JSC::ToContextRef(context), "throw new TypeError('object is not a function');", 1, &func, &exception);
    } else if (excp) {
        exception.exception_ = excp;
    }
    
    if (!exception.ShouldThow()) {
        if (JSValueIsObject(V82JSC::ToContextRef(context), result)) {
            V82JSC::makePrivateInstance(iso, V82JSC::ToContextRef(context), (JSObjectRef)result);
        }
        return ValueImpl::New(V82JSC::ToContextImpl(context), result);
    }
    
    return MaybeLocal<Value>();
}

void Function::SetName(Local<String> name)
{
    assert(0);
}

Local<Value> Function::GetName() const
{
    assert(0);
    return Local<Value>();
}

/**
 * Name inferred from variable or property assignment of this function.
 * Used to facilitate debugging and profiling of JavaScript code written
 * in an OO style, where many functions are anonymous but are assigned
 * to object properties.
 */
Local<Value> Function::GetInferredName() const
{
    assert(0);
    return Local<Value>();
}

/**
 * displayName if it is set, otherwise name if it is configured, otherwise
 * function name, otherwise inferred name.
 */
Local<Value> Function::GetDebugName() const
{
    assert(0);
    return Local<Value>();
}

/**
 * User-defined name assigned to the "displayName" property of this function.
 * Used to facilitate debugging and profiling of JavaScript code.
 */
Local<Value> Function::GetDisplayName() const
{
    assert(0);
    return Local<Value>();
}

/**
 * Returns zero based line number of function body and
 * kLineOffsetNotFound if no information available.
 */
int Function::GetScriptLineNumber() const
{
    assert(0);
    return 0;
}
/**
 * Returns zero based column number of function body and
 * kLineOffsetNotFound if no information available.
 */
int Function::GetScriptColumnNumber() const
{
    assert(0);
    return 0;
}

/**
 * Returns scriptId.
 */
int Function::ScriptId() const
{
    assert(0);
    return 0;
}

/**
 * Returns the original function if this function is bound, else returns
 * v8::Undefined.
 */
Local<Value> Function::GetBoundFunction() const
{
    assert(0);
    return Local<Value>();
}

ScriptOrigin Function::GetScriptOrigin() const
{
    assert(0);
    Local<Value> v = Local<Value>();
    ScriptOrigin so(v);
    return so;
}
