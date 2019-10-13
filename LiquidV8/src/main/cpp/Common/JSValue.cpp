/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
*/
#include <cstdlib>
#include <boost/make_shared.hpp>
#include "Common/JSValue.h"
#include "Common/Macros.h"

boost::shared_ptr<JSValue> JSValue::New(boost::shared_ptr<JSContext> context, Local<v8::Value> val)
{
    boost::shared_ptr<JSValue> value;

    if (val->IsObject()) {
        Local<Private> privateKey = v8::Private::ForApi(context->isolate(),
            String::NewFromUtf8(context->isolate(), "__JSValue_ptr"));
        Local<Object> obj = val.As<Object>();
        Local<v8::Value> identifier;
        Maybe<bool> result = obj->HasPrivate(context->Value(), privateKey);
        bool hasPrivate = false;
        if (result.IsJust() && result.FromJust()) {
            hasPrivate = obj->GetPrivate(context->Value(), privateKey).ToLocal(&identifier);
        }
        if (hasPrivate && !identifier->IsUndefined()) {
            // This object is already wrapped, let's re-use it
            return Unwrap(identifier)->shared_from_this();
        } else {
            // First time wrap.  Create it new and mark it
            value = boost::make_shared<JSValue>(context,val);
            context->retain(value);
            value->m_wrapped = true;

            obj->SetPrivate(context->Value(), privateKey, Wrap(&* value));
        }
    } else {
        value = boost::make_shared<JSValue>(context,val);
    }

    context->Group()->Manage(value);
    return value;
}

JSValue::JSValue(boost::shared_ptr<JSContext> context, Local<v8::Value> val) :
    m_context(context),
    m_isUndefined(val->IsUndefined()),
    m_isNull(val->IsNull()),
    m_wrapped(false),
    m_isObject(val->IsObject()),
    m_isNumber(val->IsNumber()),
    m_isBoolean(val->IsBoolean()),
    m_isDefunct(false)
{
    if (!m_isUndefined && !m_isNull) {
        m_value.Reset(context->isolate(), val);
    }
    if (m_isBoolean) {
        m_booleanValue = val->IsTrue();
    } else if (m_isNumber) {
        m_numberValue = val->NumberValue(context->Value()).FromJust();
    }
}

JSValue::JSValue() : m_isDefunct(false)
{
}

JSValue::~JSValue()
{
    Dispose();
}

void JSValue::Dispose()
{
    if (!m_isDefunct) {
        m_isDefunct = true;

        boost::shared_ptr<JSContext> context = m_context;
        if (context && !m_isUndefined && !m_isNull) {
            V8_ISOLATE(context->Group(), iso)
                if (m_wrapped) {
                    Local<v8::Value> local = m_value.Get(iso);
                    Local<Object> obj = local->ToObject(context->Value()).ToLocalChecked();
                    // Clear wrapper pointer if it exists, in case this object is still held by JS
                    Local<Private> privateKey = v8::Private::ForApi(iso,
                        String::NewFromUtf8(iso, "__JSValue_ptr"));
                    obj->SetPrivate(context->Value(), privateKey,
                        Local<v8::Value>::New(iso,Undefined(iso)));
                }
                m_value.Reset();
                context.reset();
            V8_UNLOCK()
        }

        m_isUndefined = true;
        m_isNull = false;
        m_wrapped = false;
        m_isObject = false;
        m_isNumber = false;
        m_isBoolean = false;
    }
}