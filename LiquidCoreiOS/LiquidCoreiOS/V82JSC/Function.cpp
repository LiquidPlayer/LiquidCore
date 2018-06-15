//
//  Function.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

using namespace v8;

/**
 * Create a function in the current execution context
 * for a given FunctionCallback.
 */
MaybeLocal<Function> Function::New(Local<Context> context, FunctionCallback callback,
                                Local<Value> data, int length,
                                ConstructorBehavior behavior)
{
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    Local<FunctionTemplate> templ = FunctionTemplate::New(isolate, callback, data, Local<Signature>(), length, behavior);
    return templ->GetFunction(context);
}


MaybeLocal<Object> Function::NewInstance(Local<Context> context, int argc, Local<Value> argv[]) const
{
    JSObjectRef func = (JSObjectRef) V82JSC::ToJSValueRef<Function>(this, context);
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    JSGlobalContextRef ctx = JSObjectGetGlobalContext((JSObjectRef)V82JSC::ToJSValueRef(this, context));
    Local<Context> cc = iso->m_global_contexts[ctx].Get(V82JSC::ToIsolate(iso));
    JSValueRef args[argc];
    for (int i=0; i<argc; i++) {
        args[i] = V82JSC::ToJSValueRef<Value>(argv[i], cc);
    }
    LocalException exception(iso);
    JSValueRef excp = 0;
    
    JSObjectRef newobj = JSObjectCallAsConstructor(V82JSC::ToContextRef(cc), func, argc, args, &excp);
    if (!newobj && !excp) {
        V82JSC::exec(V82JSC::ToContextRef(context), "throw new TypeError(_1.name + ' is not a constructor');", 1, &func, &exception);
    } else {
        exception.exception_ = excp;
    }
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(cc), newobj).As<Object>();
    }
    return MaybeLocal<Object>();
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> recv, int argc,
                                 Local<Value> argv[])
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    Context::Scope context_scope(context);

    if (iso->m_callback_depth == 0 && V82JSC::ToIsolate(iso)->GetMicrotasksPolicy() == MicrotasksPolicy::kAuto) {
        iso->m_callback_depth++;
        V82JSC::ToIsolate(iso)->RunMicrotasks();
    } else {
        iso->m_callback_depth++;
    }

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
    
    iso->m_callback_depth--;
    
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), result);
    }
    
    return MaybeLocal<Value>();
}

void Function::SetName(Local<String> name)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    
    JSContextRef ctx = V82JSC::ToContextRef(isolate->GetCurrentContext());
    JSValueRef args [] = {
        V82JSC::ToJSValueRef(this, isolate->GetCurrentContext()),
        V82JSC::ToJSValueRef(name, isolate->GetCurrentContext()),
    };
    V82JSC::exec(ctx, "delete _1['name']; Object.defineProperty(_1, 'name', {value : _2})", 2, args);
}

Local<Value> Function::GetName() const
{
    v8::Object* thiz = reinterpret_cast<v8::Object *>(const_cast<Function*>(this));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(thiz);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    TryCatch try_catch(Isolate::GetCurrent());
    MaybeLocal<Value> name = thiz->Get(isolate->GetCurrentContext(),
                                       String::NewFromUtf8(isolate, "name",
                                                           NewStringType::kNormal).ToLocalChecked());
    if (name.IsEmpty()) {
        return Undefined(isolate);
    }
    return name.ToLocalChecked();
}

/**
 * Name inferred from variable or property assignment of this function.
 * Used to facilitate debugging and profiling of JavaScript code written
 * in an OO style, where many functions are anonymous but are assigned
 * to object properties.
 */
Local<Value> Function::GetInferredName() const
{
    // JSC doesn't support inferred names
    return GetName();
}

/**
 * displayName if it is set, otherwise name if it is configured, otherwise
 * function name, otherwise inferred name.
 */
Local<Value> Function::GetDebugName() const
{
    Local<Value> name = GetDisplayName();
    if (name->IsUndefined()) name = GetName();
    if (name->IsUndefined()) name = GetInferredName();
    return name;
}

/**
 * User-defined name assigned to the "displayName" property of this function.
 * Used to facilitate debugging and profiling of JavaScript code.
 */
Local<Value> Function::GetDisplayName() const
{
    v8::Object* thiz = reinterpret_cast<v8::Object *>(const_cast<Function*>(this));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(thiz);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    TryCatch try_catch(Isolate::GetCurrent());
    MaybeLocal<Value> name = thiz->GetRealNamedProperty(isolate->GetCurrentContext(),
                                       String::NewFromUtf8(isolate, "displayName",
                                                           NewStringType::kNormal).ToLocalChecked());
    if (!name.IsEmpty()) {
        if (!name.ToLocalChecked()->IsString()) {
            name = Local<Value>();
        }
    }
    
    if (name.IsEmpty()) {
        return Undefined(isolate);
    }
    return name.ToLocalChecked();
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
