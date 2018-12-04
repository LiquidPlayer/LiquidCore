/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

uint32_t Array::Length() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSValueRef length = V82JSC::exec(ctx, "return _1.length", 1, &obj);
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
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    length = length<0 ? 0 : length;
    JSValueRef args[length];
    for (int ndx=0; ndx < length; ndx++) {
        args[ndx] = JSValueMakeUndefined(ctx);
    }
    Local<Value> o = ValueImpl::New(V82JSC::ToContextImpl(context), JSObjectMakeArray(ctx, length, args, 0));
    return o.As<Array>();
}
