//
//  Exception.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<Value> Exception::RangeError(Local<String> message)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(V82JSC::ToImpl<ValueImpl>(message));
    Isolate *isolate = V82JSC::ToIsolate(iso);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef msg = V82JSC::ToJSValueRef(message, context);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return new RangeError(_1)", 1, &msg));
}
Local<Value> Exception::ReferenceError(Local<String> message)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(V82JSC::ToImpl<ValueImpl>(message));
    Isolate *isolate = V82JSC::ToIsolate(iso);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef msg = V82JSC::ToJSValueRef(message, context);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return new ReferenceError(_1)", 1, &msg));
}
Local<Value> Exception::SyntaxError(Local<String> message)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(V82JSC::ToImpl<ValueImpl>(message));
    Isolate *isolate = V82JSC::ToIsolate(iso);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef msg = V82JSC::ToJSValueRef(message, context);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return new SyntaxError(_1)", 1, &msg));
}
Local<Value> Exception::TypeError(Local<String> message)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(V82JSC::ToImpl<ValueImpl>(message));
    Isolate *isolate = V82JSC::ToIsolate(iso);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef msg = V82JSC::ToJSValueRef(message, context);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return new TypeError(_1)", 1, &msg));
}
Local<Value> Exception::Error(Local<String> message)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(V82JSC::ToImpl<ValueImpl>(message));
    Isolate *isolate = V82JSC::ToIsolate(iso);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef msg = V82JSC::ToJSValueRef(message, context);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return new Error(_1)", 1, &msg));
}

/**
 * Creates an error message for the given exception.
 * Will try to reconstruct the original stack trace from the exception value,
 * or capture the current stack trace if not available.
 */
Local<Message> Exception::CreateMessage(Isolate* isolate, Local<Value> exception)
{
    assert(0);
    return Local<Message>();
}

/**
 * Returns the original stack trace that was captured at the creation time
 * of a given exception, or an empty handle if not available.
 */
Local<StackTrace> Exception::GetStackTrace(Local<Value> exception)
{
    assert(0);
    return Local<StackTrace>();
}
