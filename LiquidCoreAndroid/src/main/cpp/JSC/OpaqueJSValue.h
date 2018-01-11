//
// OpaqueJSValue.h
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

#ifndef LIQUIDCORE_OPAQUEJSVALUE_H
#define LIQUIDCORE_OPAQUEJSVALUE_H

#include "JSC/JSCRetainer.h"

struct OpaqueJSValue {
    public:
        static JSValueRef New(JSContextRef ctx, Local<Value> v, const JSClassDefinition* fromClass=0);
        static JSValueRef New(JSContextRef context, const char *s);
        virtual ~OpaqueJSValue();

        inline Local<Value> L() const
        {
            return value ? value->Value() : Local<Value>::New(m_ctx->Context()->isolate(), weak);
        }
        void Clean(bool fromGC=false) const;
        int Retain();
        int Release(bool cleanOnZero=true);
        inline JSContextRef Context() const { return m_ctx; }
        bool SetPrivateData(void *data);
        inline void *GetPrivateData() { return m_private_data; }
        inline void SetFinalized() { m_finalized = true; }
        inline bool HasFinalized() const { return m_finalized; }
        inline bool IsClassObject() const { return m_fromClassDefinition != nullptr; }

    protected:
        OpaqueJSValue(JSContextRef context, Local<Value> v, const JSClassDefinition* fromClass=0);

    private:
        virtual void WeakCallback();

        std::shared_ptr<JSValue> value;
        UniquePersistent<Value> weak;
        JSContextRef m_ctx;
        void *m_private_data = nullptr;
        const JSClassDefinition * m_fromClassDefinition;
        bool m_finalized = false;
        int m_count;
};

#endif //LIQUIDCORE_OPAQUEJSVALUE_H
