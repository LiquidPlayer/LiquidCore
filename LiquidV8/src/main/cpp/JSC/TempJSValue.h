/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_TEMPJSVALUE_H
#define LIQUIDCORE_TEMPJSVALUE_H

#include "Common/Common.h"
#include "JSC/Macros.h"
#include "JavaScriptCore/JavaScript.h"

class TempJSValue {
    public:
        TempJSValue() : m_value(nullptr) {}
        TempJSValue(JSContextRef context, Local<Value> v) : m_value(OpaqueJSValue::New(context,v)) {
            const_cast<OpaqueJSValue *>(m_value)->Retain();
        }
        TempJSValue(JSContextRef context, const char *s) : m_value(OpaqueJSValue::New(context,s)) {
            const_cast<OpaqueJSValue *>(m_value)->Retain();
        }
        TempJSValue(JSValueRef v) : m_value(v) {
            const_cast<OpaqueJSValue *>(m_value)->Retain();
        }
        virtual ~TempJSValue() {
            if (m_value) {
                const_cast<OpaqueJSValue *>(m_value)->Release();
            }
            m_value = nullptr;
        }
        void Set(JSContextRef context, Local<Value> v) {
            ASSERTJSC(m_value==nullptr)
            m_value = OpaqueJSValue::New(context,v);
            const_cast<OpaqueJSValue *>(m_value)->Retain();
            m_didSet = true;
        }
        void Set(JSContextRef context, const char *s) {
            ASSERTJSC(m_value==nullptr)
            m_value = OpaqueJSValue::New(context,s);
            const_cast<OpaqueJSValue *>(m_value)->Retain();
            m_didSet = true;
        }
        void Set(JSValueRef v) {
            ASSERTJSC(m_value==nullptr)
            m_value = v;
            if (v) {
                const_cast<OpaqueJSValue *>(m_value)->Retain();
                m_didSet = true;
            }
        }
        void Reset() {
            if (m_value) {
                const_cast<OpaqueJSValue *>(m_value)->Release();
            }
            m_didSet = false;
            m_value = nullptr;
        }
        JSValueRef operator*() const { return m_value; }

    protected:
        const OpaqueJSValue *m_value;
        bool m_didSet = false;
};

#endif //LIQUIDCORE_TEMPJSVALUE_H
