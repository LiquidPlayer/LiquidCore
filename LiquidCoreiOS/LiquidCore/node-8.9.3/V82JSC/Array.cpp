//
//  Array.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
