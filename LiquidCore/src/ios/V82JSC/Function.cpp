/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "Object.h"
#include "JSCPrivate.h"

using namespace V82JSC;
using v8::MaybeLocal;
using v8::Function;
using v8::Object;
using v8::Local;

/**
 * Create a function in the current execution context
 * for a given FunctionCallback.
 */
MaybeLocal<Function> Function::New(Local<Context> context, FunctionCallback callback,
                                Local<Value> data, int length,
                                ConstructorBehavior behavior,
                                SideEffectType side_effect_type)
{
    Isolate* isolate = ToIsolate(ToContextImpl(context));
    HandleScope scope(isolate);
    Local<FunctionTemplate> templ = FunctionTemplate::New(isolate, callback, data, Local<Signature>(), length, behavior);
    return templ->GetFunction(context);
}


MaybeLocal<Object> Function::NewInstance(Local<Context> context, int argc, Local<Value> argv[]) const
{
    IsolateImpl *iso = ToIsolateImpl(this);
    EscapableHandleScope scope(ToIsolate(iso));

    JSObjectRef func = (JSObjectRef) ToJSValueRef<Function>(this, context);
    JSGlobalContextRef ctx = JSContextGetGlobalContext(ToContextRef(context));
    Local<Context> cc = iso->m_global_contexts[ctx].Get(ToIsolate(iso));
    JSValueRef args[argc];
    for (int i=0; i<argc; i++) {
        args[i] = ToJSValueRef<Value>(argv[i], cc);
    }
    LocalException exception(iso);
    JSValueRef excp = 0;
    
    JSObjectRef newobj = JSObjectCallAsConstructor(ToContextRef(cc), func, argc, args, &excp);
    if (!newobj && !excp) {
        exec(ToContextRef(context), "throw new TypeError(_1.name + ' is not a constructor');", 1, &func, &exception);
    } else {
        exception.exception_ = excp;
    }
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(cc), newobj).As<Object>());
    }
    return MaybeLocal<Object>();
}

MaybeLocal<v8::Value> Function::Call(Local<Context> context,
                                 Local<Value> recv, int argc,
                                 Local<Value> argv[])
{
    IsolateImpl* iso = ToIsolateImpl(this);
    EscapableHandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    auto thread = IsolateImpl::PerThreadData::Get(iso);

    // Check if there are pending interrupts before even executing
    IsolateImpl::PollForInterrupts(ToContextRef(context), iso);

    if (thread->m_callback_depth == 0 && ToIsolate(iso)->GetMicrotasksPolicy() == MicrotasksPolicy::kAuto) {
        thread->m_callback_depth++;
        ToIsolate(iso)->RunMicrotasks();
    } else {
        thread->m_callback_depth++;
    }
    auto fimpl = ToImpl<V82JSC::Value>(this);
    Local<Value> secure_function = V82JSC::TrackedObject::SecureValue(CreateLocal<Value>(&iso->ii, fimpl));

    JSObjectRef func = (JSObjectRef) ToJSValueRef(secure_function, context);
    JSValueRef thiz = ToJSValueRef<Value>(recv, context);
    JSValueRef args[argc];
    recv = V82JSC::TrackedObject::SecureValue(recv);
    for (int i=0; i<argc; i++) {
        args[i] = ToJSValueRef<Value>(V82JSC::TrackedObject::SecureValue(argv[i]), context);
    }
    LocalException exception(iso);
    
    JSValueRef arr = JSObjectMakeArray(ToContextRef(context), argc, args, 0);
    
    JSValueRef result = 0;
    if (iso->m_disallow_js) {
        if (iso->m_on_failure == Isolate::DisallowJavascriptExecutionScope::OnFailure::CRASH_ON_FAILURE) {
            FATAL("Javascript execution disallowed");
        } else {
            *(&exception) = exec(ToContextRef(context),
                                         "return new Error('Javascript execution disallowed')", 0, nullptr);
        }
    } else {
        for (auto i=iso->m_before_call_callbacks.begin(); i!=iso->m_before_call_callbacks.end(); ++i) {
            (*i)(ToIsolate(iso));
        }
        
        JSValueRef excp = 0;
        JSValueRef inp[] = { func, thiz, arr };
        result = exec(ToContextRef(context), "return _1.apply(_2,_3)", 3, inp, &excp);
        if (!result && !excp) {
            exec(ToContextRef(context), "throw new TypeError('object is not a function');", 1, &func, &exception);
        } else if (excp) {
            exception.exception_ = excp;
        }
    }
    
    thread->m_callback_depth--;
    if (thread->m_callback_depth == 0) {
        for (auto i=iso->m_call_completed_callbacks.begin(); i!=iso->m_call_completed_callbacks.end(); ++i) {
            thread->m_callback_depth++;
            (*i)(ToIsolate(iso));
            thread->m_callback_depth--;
        }
    }
    
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), result));
    }
    
    return MaybeLocal<v8::Value>();
}

void Function::SetName(Local<String> name)
{
    HandleScope scope(ToIsolate(this));
    IsolateImpl* iso = ToIsolateImpl(this);
    Isolate* isolate = ToIsolate(iso);
    
    JSContextRef ctx = ToContextRef(isolate->GetCurrentContext());
    JSValueRef args [] = {
        ToJSValueRef(this, isolate->GetCurrentContext()),
        ToJSValueRef(name, isolate->GetCurrentContext()),
    };
    exec(ctx, "delete _1['name']; Object.defineProperty(_1, 'name', {value : _2})", 2, args);
}

Local<v8::Value> Function::GetName() const
{
    IsolateImpl* iso = ToIsolateImpl(this);
    Isolate* isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    
    v8::Object* thiz = reinterpret_cast<v8::Object *>(const_cast<Function*>(this));
    TryCatch try_catch(isolate);
    MaybeLocal<Value> name = thiz->Get(isolate->GetCurrentContext(),
                                       String::NewFromUtf8(isolate, "name",
                                                           NewStringType::kNormal).ToLocalChecked());
    if (name.IsEmpty()) {
        return scope.Escape(Undefined(isolate));
    }
    return scope.Escape(name.ToLocalChecked());
}

/**
 * Name inferred from variable or property assignment of this function.
 * Used to facilitate debugging and profiling of JavaScript code written
 * in an OO style, where many functions are anonymous but are assigned
 * to object properties.
 */
Local<v8::Value> Function::GetInferredName() const
{
    // JSC doesn't support inferred names
    return GetName();
}

/**
 * displayName if it is set, otherwise name if it is configured, otherwise
 * function name, otherwise inferred name.
 */
Local<v8::Value> Function::GetDebugName() const
{
    EscapableHandleScope scope(ToIsolate(this));
    Local<Value> name = GetDisplayName();
    if (name->IsUndefined()) name = GetName();
    if (name->IsUndefined()) name = GetInferredName();
    return scope.Escape(name);
}

/**
 * User-defined name assigned to the "displayName" property of this function.
 * Used to facilitate debugging and profiling of JavaScript code.
 */
Local<v8::Value> Function::GetDisplayName() const
{
    v8::Object* thiz = reinterpret_cast<v8::Object *>(const_cast<Function*>(this));
    IsolateImpl* iso = ToIsolateImpl(thiz);
    Isolate* isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    
    TryCatch try_catch(isolate);
    MaybeLocal<Value> name = thiz->GetRealNamedProperty(isolate->GetCurrentContext(),
                                       String::NewFromUtf8(isolate, "displayName",
                                                           NewStringType::kNormal).ToLocalChecked());
    if (!name.IsEmpty()) {
        if (!name.ToLocalChecked()->IsString()) {
            name = Local<Value>();
        }
    }
    
    if (name.IsEmpty()) {
        return scope.Escape(Undefined(isolate));
    }
    return scope.Escape(name.ToLocalChecked());
}

/**
 * Returns zero based line number of function body and
 * kLineOffsetNotFound if no information available.
 */
int Function::GetScriptLineNumber() const
{
    // Don't think this is possible in JSC
    return kLineOffsetNotFound;
}
/**
 * Returns zero based column number of function body and
 * kLineOffsetNotFound if no information available.
 */
int Function::GetScriptColumnNumber() const
{
    // Don't think this is possible in JSC
    return kLineOffsetNotFound;
}

const int Function::kLineOffsetNotFound = -1;

/**
 * Returns scriptId.
 */
int Function::ScriptId() const
{
    // Don't think this is possible in JSC
    return 0;
}

/**
 * Returns the original function if this function is bound, else returns
 * v8::Undefined.
 */
Local<v8::Value> Function::GetBoundFunction() const
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    auto impl = ToImpl<V82JSC::Value>(this);
    
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(impl->GetNullContext(), (JSObjectRef)impl->m_value);
    if (wrap && wrap->m_bound_function) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(OperatingContext(isolate)),
                                           wrap->m_bound_function));
    }

    return scope.Escape(Undefined(isolate));
}

v8::ScriptOrigin Function::GetScriptOrigin() const
{
    // Don't think this is possible in JSC
    Local<Value> v = Local<Value>();
    ScriptOrigin so(v);
    return so;
}
