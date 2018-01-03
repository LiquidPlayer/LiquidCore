//
// TempException.h
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
