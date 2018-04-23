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
    JSValueRef arg = JSValueMakeNumber(context->m_ctxRef, value);
    JSObjectRef func = JSFUNC(NewNumberObject, "return new Number(v)", context);
    return ValueImpl::New(context, JSObjectCallAsFunction(context->m_ctxRef, func, 0, 1, &arg, 0));
}

double NumberObject::ValueOf() const
{
    auto c = V82JSC::ToContextImpl(this);
    auto v = V82JSC::ToJSValueRef<v8::Value>(this, _local<v8::Context>(c).toLocal());
    JSValueRef exception = nullptr;
    double ret = JSValueToNumber(c->m_ctxRef,
                                 JSObjectCallAsFunction(c->m_ctxRef, JSFUNC(ValueOf, VALUE_OF_CODE, c),
                                                        0, 1, &v, &exception), &exception);
    assert(exception==nullptr);
    return ret;
}

Local<Value> BooleanObject::New(Isolate* isolate, bool value)
{
    ContextImpl *context = reinterpret_cast<IsolateImpl*>(isolate)->m_defaultContext;
    JSValueRef arg = JSValueMakeBoolean(context->m_ctxRef, value);
    JSObjectRef func = JSFUNC(NewBooleanObject, "return new Boolean(v)", context);
    return ValueImpl::New(context, JSObjectCallAsFunction(context->m_ctxRef, func, 0, 1, &arg, 0));
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
    return ValueImpl::New(context, JSObjectCallAsFunction(context->m_ctxRef, func, 0, 1, &v, 0));
}

Local<String> StringObject::ValueOf() const
{
    auto c = V82JSC::ToContextImpl(this);
    auto v = V82JSC::ToJSValueRef<v8::Value>(this, _local<v8::Context>(c).toLocal());
    JSValueRef exception = nullptr;
    JSValueRef rval = JSObjectCallAsFunction(c->m_ctxRef, JSFUNC(ValueOf, VALUE_OF_CODE, c), 0, 1, &v, &exception);
    assert(exception==nullptr);
    JSStringRef ret = JSValueToStringCopy(c->m_ctxRef, rval, &exception);
    assert(exception==nullptr);
    return ValueImpl::New(reinterpret_cast<Isolate*>(c->m_isolate), ret);
}

Local<Value> SymbolObject::New(Isolate* isolate, Local<Symbol> value)
{
    ValueImpl* symbol = V82JSC::ToImpl<ValueImpl>(value);
    JSValueRef symbol_object = V82JSC::exec(symbol->m_context->m_ctxRef, "return Object(_1)", 1, &symbol->m_value);
    
    return ValueImpl::New(symbol->m_context, symbol_object);
}

Local<Symbol> SymbolObject::ValueOf() const
{
    ContextImpl* ctximpl = V82JSC::ToContextImpl<SymbolObject>(this);
    Local<Context> context = _local<Context>(ctximpl).toLocal();
    JSValueRef symbol_object = V82JSC::ToJSValueRef(this, context);
    
    return ValueImpl::New(ctximpl, V82JSC::exec(ctximpl->m_ctxRef, "return _1.valueOf()", 1, &symbol_object)).As<Symbol>();
}

