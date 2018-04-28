//
//  PrimitiveObjects.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

#define VALUE_OF_CODE "return (typeof v === 'object' && v !== null) ? v.valueOf() : v"

Local<Value> NumberObject::New(Isolate* isolate, double value)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    JSValueRef arg = JSValueMakeNumber(ctx, value);
    JSObjectRef func = JSFUNC(NewNumberObject, "return new Number(v)", V82JSC::ToContextImpl(context));
    return ValueImpl::New(V82JSC::ToContextImpl(context), JSObjectCallAsFunction(ctx, func, 0, 1, &arg, 0));
}

double NumberObject::ValueOf() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    auto v = V82JSC::ToJSValueRef(this, context);
    JSValueRef exception = nullptr;
    double ret = JSValueToNumber(ctx,
                                 JSObjectCallAsFunction(ctx, JSFUNC(ValueOf, VALUE_OF_CODE, V82JSC::ToContextImpl(context)),
                                                        0, 1, &v, &exception), &exception);
    assert(exception==nullptr);
    return ret;
}

Local<Value> BooleanObject::New(Isolate* isolate, bool value)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    JSValueRef arg = JSValueMakeBoolean(ctx, value);
    JSObjectRef func = JSFUNC(NewBooleanObject, "return new Boolean(v)", V82JSC::ToContextImpl(context));
    return ValueImpl::New(V82JSC::ToContextImpl(context), JSObjectCallAsFunction(ctx, func, 0, 1, &arg, 0));
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
    JSObjectRef func = JSFUNC(NewStringObject, "return new String(v)", V82JSC::ToContextImpl(context));
    return ValueImpl::New(V82JSC::ToContextImpl(context), JSObjectCallAsFunction(ctx, func, 0, 1, &v, 0));
}

Local<String> StringObject::ValueOf() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    auto v = V82JSC::ToJSValueRef(this, context);

    JSValueRef exception = nullptr;
    JSValueRef rval = JSObjectCallAsFunction(ctx, JSFUNC(ValueOf, VALUE_OF_CODE, V82JSC::ToContextImpl(context)), 0, 1, &v, &exception);
    assert(exception==nullptr);
    JSStringRef ret = JSValueToStringCopy(ctx, rval, &exception);
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

