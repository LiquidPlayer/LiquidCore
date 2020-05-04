/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using v8::Array;
using v8::Local;

uint32_t Array::Length() const
{
    auto isolate = ToIsolate(this);
    HandleScope scope(isolate);
    
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    JSValueRef length = exec(ctx, "return _1.length", 1, &obj);
    uint32_t len = 0;
    if (length) {
        JSValueRef excp = 0;
        len = JSValueToNumber(ctx, length, &excp);
        assert(excp==0);
    }
    return len;
}

/**
 * Creates a JavaScript array with the given length. If the length
 * is negative the returned array will have length 0.
 */
Local<Array> Array::New(Isolate* isolate, int length)
{
    EscapableHandleScope scope(isolate);

    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    length = length<0 ? 0 : length;
    JSValueRef args[length];
    for (int ndx=0; ndx < length; ndx++) {
        args[ndx] = JSValueMakeUndefined(ctx);
    }
    Local<Value> o = V82JSC::Value::New(ToContextImpl(context),
                                        JSObjectMakeArray(ctx, length, args, 0));
    return scope.Escape(o.As<Array>());
}
