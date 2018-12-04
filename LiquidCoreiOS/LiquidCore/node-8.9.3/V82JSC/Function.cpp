/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
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
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    EscapableHandleScope scope(V82JSC::ToIsolate(iso));

    JSObjectRef func = (JSObjectRef) V82JSC::ToJSValueRef<Function>(this, context);
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
        return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(cc), newobj).As<Object>());
    }
    return MaybeLocal<Object>();
}

MaybeLocal<Value> Function::Call(Local<Context> context,
                                 Local<Value> recv, int argc,
                                 Local<Value> argv[])
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    EscapableHandleScope scope(V82JSC::ToIsolate(iso));
    Context::Scope context_scope(context);
    auto thread = IsolateImpl::PerThreadData::Get(iso);

    // Check if there are pending interrupts before even executing
    IsolateImpl::PollForInterrupts(V82JSC::ToContextRef(context), iso);

    if (thread->m_callback_depth == 0 && V82JSC::ToIsolate(iso)->GetMicrotasksPolicy() == MicrotasksPolicy::kAuto) {
        thread->m_callback_depth++;
        V82JSC::ToIsolate(iso)->RunMicrotasks();
    } else {
        thread->m_callback_depth++;
    }
    ValueImpl *fimpl = V82JSC::ToImpl<ValueImpl>(this);
    Local<Value> secure_function = TrackedObjectImpl::SecureValue(V82JSC::CreateLocal<Value>(&iso->ii, fimpl));

    JSObjectRef func = (JSObjectRef) V82JSC::ToJSValueRef(secure_function, context);
    JSValueRef thiz = V82JSC::ToJSValueRef<Value>(recv, context);
    JSValueRef args[argc];
    recv = TrackedObjectImpl::SecureValue(recv);
    for (int i=0; i<argc; i++) {
        args[i] = V82JSC::ToJSValueRef<Value>(TrackedObjectImpl::SecureValue(argv[i]), context);
    }
    LocalException exception(iso);
    
    JSValueRef result = 0;
    if (iso->m_disallow_js) {
        if (iso->m_on_failure == Isolate::DisallowJavascriptExecutionScope::OnFailure::CRASH_ON_FAILURE) {
            FATAL("Javascript execution disallowed");
        } else {
            *(&exception) = V82JSC::exec(V82JSC::ToContextRef(context),
                                         "return new Error('Javascript execution disallowed')", 0, nullptr);
        }
    } else {
        for (auto i=iso->m_before_call_callbacks.begin(); i!=iso->m_before_call_callbacks.end(); ++i) {
            (*i)(V82JSC::ToIsolate(iso));
        }
        
        JSValueRef excp = 0;
        result = JSObjectCallAsFunction(V82JSC::ToContextRef(context), func, (JSObjectRef)thiz, argc, args, &excp);
        if (!result && !excp) {
            V82JSC::exec(V82JSC::ToContextRef(context), "throw new TypeError('object is not a function');", 1, &func, &exception);
        } else if (excp) {
            exception.exception_ = excp;
        }
    }
    
    thread->m_callback_depth--;
    if (thread->m_callback_depth == 0) {
        for (auto i=iso->m_call_completed_callbacks.begin(); i!=iso->m_call_completed_callbacks.end(); ++i) {
            thread->m_callback_depth++;
            (*i)(V82JSC::ToIsolate(iso));
            thread->m_callback_depth--;
        }
    }
    
    if (!exception.ShouldThow()) {
        return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(context), result));
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
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    Isolate* isolate = V82JSC::ToIsolate(iso);
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
    EscapableHandleScope scope(V82JSC::ToIsolate(this));
    Local<Value> name = GetDisplayName();
    if (name->IsUndefined()) name = GetName();
    if (name->IsUndefined()) name = GetInferredName();
    return scope.Escape(name);
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
Local<Value> Function::GetBoundFunction() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    EscapableHandleScope scope(isolate);
    ValueImpl* impl = V82JSC::ToImpl<ValueImpl>(this);
    
    TrackedObjectImpl *wrap = getPrivateInstance(impl->GetNullContext(), (JSObjectRef)impl->m_value);
    if (wrap && wrap->m_bound_function) {
        return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(V82JSC::OperatingContext(isolate)),
                                           wrap->m_bound_function));
    }

    return scope.Escape(Undefined(isolate));
}

ScriptOrigin Function::GetScriptOrigin() const
{
    // Don't think this is possible in JSC
    Local<Value> v = Local<Value>();
    ScriptOrigin so(v);
    return so;
}
