//
// TempJSValue.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

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

#ifndef LIQUIDCORE_TEMPJSVALUE_H
#define LIQUIDCORE_TEMPJSVALUE_H

#include "Common/Common.h"
#include "JSC/Macros.h"
#include "JavaScriptCore/Javascript.h"

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
                const_cast<OpaqueJSValue *>(m_value)->Release(!m_value->IsClassObject());
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
                const_cast<OpaqueJSValue *>(m_value)->Release(!m_value->IsClassObject());
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
