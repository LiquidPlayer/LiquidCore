//
//  PrimitiveObjects.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

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
    
    JSValueRef v = V82JSC::ToJSValueRef<String>(value, Local<Context>());
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
    return ValueImpl::New(V82JSC::ToIsolate(V82JSC::ToContextImpl(context)->m_isolate), ret);
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

