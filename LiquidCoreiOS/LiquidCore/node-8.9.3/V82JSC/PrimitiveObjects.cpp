//
//  PrimitiveObjects.cpp
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

#define VALUE_OF_CODE "return (typeof _1 === 'object' && _1 !== null) ? _1.valueOf() : v"

Local<Value> NumberObject::New(Isolate* isolate, double value)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    JSValueRef arg = JSValueMakeNumber(ctx, value);
    JSValueRef obj = V82JSC::exec(ctx, "return new Number(_1)", 1, &arg);
    return ValueImpl::New(V82JSC::ToContextImpl(context), obj);
}

double NumberObject::ValueOf() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    auto v = V82JSC::ToJSValueRef(this, context);
    JSValueRef exception = nullptr;
    double ret = JSValueToNumber(ctx, V82JSC::exec(ctx, VALUE_OF_CODE, 1, &v), &exception);
    assert(exception==nullptr);
    return ret;
}

Local<Value> BooleanObject::New(Isolate* isolate, bool value)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    JSValueRef arg = JSValueMakeBoolean(ctx, value);
    JSValueRef obj = V82JSC::exec(ctx, "return new Boolean(_1)", 1, &arg);
    return ValueImpl::New(V82JSC::ToContextImpl(context), obj);
}
bool BooleanObject::ValueOf() const
{
    return IS(ValueOf,VALUE_OF_CODE);
}

Local<Value> StringObject::New(Local<String> value)
{
    Local<Context> context = V82JSC::ToCurrentContext(*value);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    JSValueRef v = V82JSC::ToJSValueRef<String>(value, context);
    JSValueRef obj = V82JSC::exec(ctx, "return new String(_1)", 1, &v);
    return ValueImpl::New(V82JSC::ToContextImpl(context), obj);
}

Local<String> StringObject::ValueOf() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    auto v = V82JSC::ToJSValueRef(this, context);

    JSValueRef exception = nullptr;
    JSStringRef ret = JSValueToStringCopy(ctx, V82JSC::exec(ctx, VALUE_OF_CODE, 1, &v), &exception);
    assert(exception==nullptr);
    return StringImpl::New(V82JSC::ToIsolate(this), ret);
}

Local<Value> SymbolObject::New(Isolate* isolate, Local<Symbol> value)
{
    Local<Context> context = V82JSC::ToCurrentContext(*value);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef symbol = V82JSC::ToJSValueRef(value, context);
    JSValueRef symbol_object = V82JSC::exec(ctx, "return Object(_1)", 1, &symbol);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), symbol_object);
}

Local<Symbol> SymbolObject::ValueOf() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef symbol_object = V82JSC::ToJSValueRef(this, context);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return _1.valueOf()", 1, &symbol_object)).As<Symbol>();
}

