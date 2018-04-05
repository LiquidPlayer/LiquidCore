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
    ContextImpl *context = reinterpret_cast<IsolateImpl*>(isolate)->m_defaultContext;
    JSValueRef arg = JSValueMakeNumber(context->m_context, value);
    JSObjectRef func = JSFUNC(NewNumberObject, "return new Number(v)", context);
    return ValueImpl::New(context, JSObjectCallAsFunction(context->m_context, func, 0, 1, &arg, 0));
}

double NumberObject::ValueOf() const
{
    auto c = V82JSC::ToContextImpl(this);
    auto v = V82JSC::ToJSValueRef<v8::Value>(this, _local<v8::Context>(c).toLocal());
    JSValueRef exception = nullptr;
    double ret = JSValueToNumber(c->m_context,
                                 JSObjectCallAsFunction(c->m_context, JSFUNC(ValueOf, VALUE_OF_CODE, c),
                                                        0, 1, &v, &exception), &exception);
    assert(exception==nullptr);
    return ret;
}

Local<Value> BooleanObject::New(Isolate* isolate, bool value)
{
    ContextImpl *context = reinterpret_cast<IsolateImpl*>(isolate)->m_defaultContext;
    JSValueRef arg = JSValueMakeBoolean(context->m_context, value);
    JSObjectRef func = JSFUNC(NewBooleanObject, "return new Boolean(v)", context);
    return ValueImpl::New(context, JSObjectCallAsFunction(context->m_context, func, 0, 1, &arg, 0));
}
bool BooleanObject::ValueOf() const
{
    return IS(ValueOf,VALUE_OF_CODE);
}

Local<Value> StringObject::New(Local<String> value)
{
    JSValueRef v = V82JSC::ToJSValueRef<String>(value, Local<Context>());
    ContextImpl *context = V82JSC::ToContextImpl<String>(*value);
    JSObjectRef func = JSFUNC(NewStringObject, "return new String(v)", context);
    return ValueImpl::New(context, JSObjectCallAsFunction(context->m_context, func, 0, 1, &v, 0));
}

Local<String> StringObject::ValueOf() const
{
    auto c = V82JSC::ToContextImpl(this);
    auto v = V82JSC::ToJSValueRef<v8::Value>(this, _local<v8::Context>(c).toLocal());
    JSValueRef exception = nullptr;
    JSValueRef rval = JSObjectCallAsFunction(c->m_context, JSFUNC(ValueOf, VALUE_OF_CODE, c), 0, 1, &v, &exception);
    assert(exception==nullptr);
    JSStringRef ret = JSValueToStringCopy(c->m_context, rval, &exception);
    assert(exception==nullptr);
    return ValueImpl::New(reinterpret_cast<Isolate*>(c->isolate), ret);
}

Local<Value> SymbolObject::New(Isolate* isolate, Local<Symbol> value)
{
    return Local<Value>();
}

Local<Symbol> SymbolObject::ValueOf() const
{
    return Local<Symbol>();
}

