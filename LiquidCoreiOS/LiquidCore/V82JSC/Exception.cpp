/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "Message.h"

using namespace V82JSC;
using v8::Local;
using v8::Exception;

Local<v8::Value> Exception::RangeError(Local<String> message)
{
    IsolateImpl *iso = ToIsolateImpl(ToImpl<Value>(message));
    Isolate *isolate = ToIsolate(iso);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef msg = ToJSValueRef(message, context);
    
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return new RangeError(_1)", 1, &msg));
}
Local<v8::Value> Exception::ReferenceError(Local<String> message)
{
    IsolateImpl *iso = ToIsolateImpl(ToImpl<Value>(message));
    Isolate *isolate = ToIsolate(iso);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef msg = ToJSValueRef(message, context);
    
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return new ReferenceError(_1)", 1, &msg));
}
Local<v8::Value> Exception::SyntaxError(Local<String> message)
{
    IsolateImpl *iso = ToIsolateImpl(ToImpl<Value>(message));
    Isolate *isolate = ToIsolate(iso);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef msg = ToJSValueRef(message, context);
    
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return new SyntaxError(_1)", 1, &msg));
}
Local<v8::Value> Exception::TypeError(Local<String> message)
{
    IsolateImpl *iso = ToIsolateImpl(ToImpl<Value>(message));
    Isolate *isolate = ToIsolate(iso);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef msg = ToJSValueRef(message, context);
    
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return new TypeError(_1)", 1, &msg));
}
Local<v8::Value> Exception::Error(Local<String> message)
{
    IsolateImpl *iso = ToIsolateImpl(ToImpl<Value>(message));
    Isolate *isolate = ToIsolate(iso);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef msg = ToJSValueRef(message, context);
    
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return new Error(_1)", 1, &msg));
}

/**
 * Creates an error message for the given exception.
 * Will try to reconstruct the original stack trace from the exception value,
 * or capture the current stack trace if not available.
 */
Local<v8::Message> Exception::CreateMessage(Isolate* isolate, Local<Value> exception)
{
    IsolateImpl *iso = ToIsolateImpl(isolate);
    Local<Context> context = OperatingContext(isolate);
    auto thread = IsolateImpl::PerThreadData::Get(iso);
 
    Local<Script> script;
    if (!thread->m_running_scripts.empty()) {
        script = thread->m_running_scripts.top();
    }

    auto msgi = V82JSC::Message::New(iso, ToJSValueRef(exception, context), script, 0);
    Local<v8::Message> msg = CreateLocal<v8::Message>(&iso->ii, msgi);

    return msg;
}

/**
 * Returns the original stack trace that was captured at the creation time
 * of a given exception, or an empty handle if not available.
 */
Local<v8::StackTrace> Exception::GetStackTrace(Local<Value> exception)
{
    Isolate* isolate = Isolate::GetCurrent();
    
    return CreateMessage(isolate, exception)->GetStackTrace();
}
