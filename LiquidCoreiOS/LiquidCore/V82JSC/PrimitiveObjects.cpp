/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "StringImpl.h"

using namespace V82JSC;
using namespace v8;

#define VALUE_OF_CODE "return (typeof _1 === 'object' && _1 !== null) ? _1.valueOf() : v"

Local<v8::Value> NumberObject::New(Isolate* isolate, double value)
{
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);

    JSValueRef arg = JSValueMakeNumber(ctx, value);
    JSValueRef obj = exec(ctx, "return new Number(_1)", 1, &arg);
    return V82JSC::Value::New(ToContextImpl(context), obj);
}

double NumberObject::ValueOf() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    auto v = ToJSValueRef(this, context);
    JSValueRef exception = nullptr;
    double ret = JSValueToNumber(ctx, exec(ctx, VALUE_OF_CODE, 1, &v), &exception);
    assert(exception==nullptr);
    return ret;
}

Local<v8::Value> BooleanObject::New(Isolate* isolate, bool value)
{
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);

    JSValueRef arg = JSValueMakeBoolean(ctx, value);
    JSValueRef obj = exec(ctx, "return new Boolean(_1)", 1, &arg);
    return V82JSC::Value::New(ToContextImpl(context), obj);
}

bool BooleanObject::ValueOf() const
{
    v8::HandleScope scope(v8::Isolate::GetCurrent());
    v8::Local<v8::Context> context = ToCurrentContext(this);
    auto ctx = ToContextRef(context);
    auto v = ToJSValueRef<v8::Value>(this, context);
    
    JSValueRef b = exec(ctx, VALUE_OF_CODE, 1, &v);
    bool ret = JSValueToBoolean(ctx, b);
    return ret;
}

Local<v8::Value> StringObject::New(Local<String> value)
{
    Local<Context> context = ToCurrentContext(*value);
    JSContextRef ctx = ToContextRef(context);
    
    JSValueRef v = ToJSValueRef<String>(value, context);
    JSValueRef obj = exec(ctx, "return new String(_1)", 1, &v);
    return V82JSC::Value::New(ToContextImpl(context), obj);
}

Local<v8::String> StringObject::ValueOf() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    auto v = ToJSValueRef(this, context);

    JSValueRef exception = nullptr;
    JSStringRef ret = JSValueToStringCopy(ctx, exec(ctx, VALUE_OF_CODE, 1, &v), &exception);
    assert(exception==nullptr);
    return V82JSC::String::New(ToIsolate(this), ret);
}

Local<v8::Value> SymbolObject::New(Isolate* isolate, Local<Symbol> value)
{
    Local<Context> context = ToCurrentContext(*value);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef symbol = ToJSValueRef(value, context);
    JSValueRef symbol_object = exec(ctx, "return Object(_1)", 1, &symbol);
    
    return V82JSC::Value::New(ToContextImpl(context), symbol_object);
}

Local<Symbol> SymbolObject::ValueOf() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef symbol_object = ToJSValueRef(this, context);
    
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return _1.valueOf()", 1, &symbol_object)).As<Symbol>();
}

