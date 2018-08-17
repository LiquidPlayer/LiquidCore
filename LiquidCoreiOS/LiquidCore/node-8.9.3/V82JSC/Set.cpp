//
//  Set.cpp
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

size_t Set::Size() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);

    JSStringRef ssize = JSStringCreateWithUTF8CString("size");
    JSValueRef excp = 0;
    JSValueRef size = JSObjectGetProperty(ctx, (JSObjectRef)obj, ssize, &excp);
    assert(excp==0);
    return JSValueToNumber(ctx, size, 0);
}
void Set::Clear()
{
    reinterpret_cast<Map*>(this)->Clear();
}
MaybeLocal<Set> Set::Add(Local<Context> context, Local<Value> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl, Set>(this);
    JSValueRef args[] = {
        obj,
        V82JSC::ToJSValueRef(key, context),
    };
    V82JSC::exec(ctx, "_1.add(_2)", 2, args, &exception);
    if (exception.ShouldThow()) return MaybeLocal<Set>();
    return V82JSC::CreateLocal<Set>(V82JSC::ToIsolate(iso), impl);
}
Maybe<bool> Set::Has(Local<Context> context, Local<Value> key)
{
    return reinterpret_cast<Map*>(this)->Has(context, key);
}
Maybe<bool> Set::Delete(Local<Context> context, Local<Value> key)
{
    return reinterpret_cast<Map*>(this)->Delete(context, key);
}

/**
 * Returns an array of the keys in this Set.
 */
Local<Array> Set::AsArray() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    return ValueImpl::New(V82JSC::ToContextImpl(context),
                          V82JSC::exec(ctx, "var r = []; _1.forEach((v)=>r.push(v)); return r", 1, &obj)).As<Array>();
}

/**
 * Creates a new empty Set.
 */
Local<Set> Set::New(Isolate* isolate)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    return ValueImpl::New(V82JSC::ToContextImpl(context),
                          V82JSC::exec(V82JSC::ToContextRef(context),
                                       "return new Set()", 0, 0)).As<Set>();
}
