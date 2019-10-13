/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
            EscapableHandleScope scope(Isolate::GetCurrent());
            boost::shared_ptr<JSValue> value = m_value;
            return value ? scope.Escape(value->Value()) :
                   scope.Escape(Local<Value>::New(m_ctx->Context()->isolate(), weak));
        }
        bool Clean() const;
        int Retain();
        int Release();
        inline JSContextRef Context() const { return m_ctx; }
        bool SetPrivateData(void *data);
        inline void *GetPrivateData() { return m_private_data; }
        inline void SetFinalized() { m_finalized = true; }
        inline bool HasFinalized() const { return m_finalized; }
        inline bool IsClassObject() const { return m_fromClassDefinition != nullptr; }
        inline bool IsDefunct() const { return m_count == 0; }
        inline const JSClassDefinition *Definition() const { return m_fromClassDefinition; }
        inline void ClearWeak() { weak.Reset(); }

    protected:
        OpaqueJSValue(JSContextRef context, Local<Value> v, const JSClassDefinition* fromClass=0);

    private:
        virtual void WeakCallback();

        boost::atomic_shared_ptr<JSValue> m_value;
        UniquePersistent<Value> weak;
        JSContextRef m_ctx;
        void *m_private_data = nullptr;
        const JSClassDefinition * m_fromClassDefinition;
        bool m_finalized = false;
        int m_count;
};

#endif //LIQUIDCORE_OPAQUEJSVALUE_H
