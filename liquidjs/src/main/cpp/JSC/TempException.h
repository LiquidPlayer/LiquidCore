/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_TEMPEXCEPTION_H
#define LIQUIDCORE_TEMPEXCEPTION_H

#include "JSC/TempJSValue.h"

class TempException : public TempJSValue {
    public:
        TempException(JSValueRef *exceptionRef) : TempJSValue() {
            m_exceptionRef = exceptionRef;
        }
        virtual ~TempException() {
            if (m_exceptionRef) {
                *m_exceptionRef = m_value;
            }
            if (m_value && m_didSet) {
                const_cast<OpaqueJSValue *>(m_value)->Release(!m_exceptionRef &&
                    !m_value->IsClassObject());
            }
            m_value = nullptr;
        }
        void Reset() {
            ASSERTJSC(false);
        }
        JSValueRef* operator&() { return &m_value; }

    private:
        JSValueRef *m_exceptionRef;
};

#endif //LIQUIDCORE_TEMPEXCEPTION_H
