//
// JSC.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
#ifndef NODEDROID_JSC_H
#define NODEDROID_JSC_H

#include "common.h"

#include <vector>
#include <string>

#define OBJECT_DATA_DEFINITION   0
#define OBJECT_DATA_CONTEXT      1
#define OBJECT_DATA_FIELDS       2

#define FUNCTION_DATA_CLASS      2
#define FUNCTION_DATA_FIELDS     3

#define INSTANCE_OBJECT_CLASS    0
#define INSTANCE_OBJECT_JSOBJECT 1
#define INSTANCE_OBJECT_FIELDS   2

#define OpaqueJSContextGroup            ContextGroup
#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       OpaqueJSValue

#include "JavaScriptCore/JavaScript.h"

#define ASSERT(x) if(!(x)) \
    __android_log_assert("conditional", "ASSERT FAILED", "%s(%d) : %s", __FILE__, __LINE__, #x);

class OpaqueJSContext : public Retainer {
    public:
        OpaqueJSContext(JSContext *ctx);
        virtual ~OpaqueJSContext();
        JSContext *Context() const { return m_context; }
        virtual void MarkForCollection(JSValueRef value);
        virtual void MarkCollected(JSValueRef value);
        virtual void ForceGC();

    private:
        virtual void GCCallback(GCType type, GCCallbackFlags flags);

        JSContext *m_context;
        std::list<JSValueRef> m_collection;
        std::recursive_mutex m_gc_lock;

        static void StaticGCCallback(GCType type, GCCallbackFlags flags, void*data) {
            ((OpaqueJSContext*)data)->GCCallback(type,flags);
        }
};

class OpaqueJSString : public Retainer {
    public:
        OpaqueJSString(Local<String> string);
        OpaqueJSString(const JSChar * chars, size_t numChars);
        OpaqueJSString(const char * chars);
        virtual ~OpaqueJSString();
        virtual Local<String> Value(Isolate *);
        virtual const JSChar * Chars();
        virtual size_t Size();
        virtual size_t Utf8Bytes();
        virtual void Utf8String(std::string&);
        virtual bool Equals(OpaqueJSString& other);

    private:
        std::vector<unsigned short> backstore;
        bool m_isNull;
};

class OpaqueJSClass : public Retainer {
    public:
        OpaqueJSClass(const JSClassDefinition *definition);
        virtual ~OpaqueJSClass();
        virtual const JSClassDefinition * Definition() { return m_definition; }

        virtual Local<ObjectTemplate> NewTemplate(Local<Object> *data);
        virtual JSObjectRef InitInstance(JSContextRef ctx, Local<Object> instance,
            Local<Object> data, void *privateData);

        static void CallAsFunction(const FunctionCallbackInfo< Value > &);
        static void HasInstanceFunctionCallHandler(const FunctionCallbackInfo< Value > &);

    private:
        static void StaticFunctionCallHandler(const FunctionCallbackInfo< Value > &);
        static void ConvertFunctionCallHandler(const FunctionCallbackInfo< Value > &);
        static void Finalize(const WeakCallbackInfo<UniquePersistent<Object>>&);

        static void NamedPropertyGetter(Local< String >, const PropertyCallbackInfo< Value > &);
        static void NamedPropertyQuerier(Local< String >, const PropertyCallbackInfo< Integer > &);
        static void NamedPropertySetter(Local<String>,Local<Value>,
            const PropertyCallbackInfo<Value>&);
        static void NamedPropertyDeleter(Local< String >, const PropertyCallbackInfo< Boolean > &);
        static void NamedPropertyEnumerator(const PropertyCallbackInfo< Array > &);

        static void IndexedPropertyGetter(uint32_t, const PropertyCallbackInfo< Value > &);
        static void IndexedPropertyQuerier(uint32_t, const PropertyCallbackInfo< Integer > &);
        static void IndexedPropertySetter(uint32_t,Local<Value>,const PropertyCallbackInfo<Value>&);
        static void IndexedPropertyDeleter(uint32_t, const PropertyCallbackInfo< Boolean > &);
        static void IndexedPropertyEnumerator(const PropertyCallbackInfo< Array > &);

        static void ProtoPropertyGetter(Local< String >, const PropertyCallbackInfo< Value > &);
        static void ProtoPropertyQuerier(Local< String >, const PropertyCallbackInfo< Integer > &);
        static void ProtoPropertyEnumerator(const PropertyCallbackInfo< Array > &);

        virtual bool IsFunction();
        virtual bool IsConstructor();
        JSClassDefinition *m_definition;
        JSObjectRef m_classObject;
};

class OpaqueJSValue {
    public:
        static OpaqueJSValue* New(JSContextRef ctx, Local<Value> v,
            const JSClassDefinition* fromClass=0)
        {
            OpaqueJSValue *out = nullptr;
            V8_ISOLATE(ctx->Context()->Group(), isolate)
                if (v->IsObject() && !fromClass) {
                    Local<v8::Context> context = ctx->Context()->Value();
                    Context::Scope(ctx->Context()->Value());
                    Local<Object> o = v->ToObject(context).ToLocalChecked();
                    o = o->StrictEquals(context->Global()) && \
                        !o->GetPrototype()->ToObject(context).IsEmpty() && \
                        o->GetPrototype()->ToObject(context).ToLocalChecked()->InternalFieldCount()?\
                        o->GetPrototype()->ToObject(context).ToLocalChecked() : \
                        o;
                    if (o->InternalFieldCount() > INSTANCE_OBJECT_JSOBJECT) {
                        out=
                            reinterpret_cast<OpaqueJSValue*>(
                                o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_JSOBJECT));
                        out->Retain();
                    }
                }
            V8_UNLOCK()
            if (!out) {
                out = new OpaqueJSValue(ctx, v, fromClass);
            }
            return out;
        }
        static OpaqueJSValue* New(JSContextRef context, const char *s)
        {
            ASSERT(s);
            Local<String> local = String::NewFromUtf8(context->Context()->isolate(),s);
            return new OpaqueJSValue(context, local);
        }
        virtual ~OpaqueJSValue() {
            V8_ISOLATE(m_ctx->Context()->Group(), isolate)
                Local<Value> v = L();
                if (*v && v->IsObject()) {
                    Local<v8::Context> context = m_ctx->Context()->Value();
                    Local<Object> o = v->ToObject(context).ToLocalChecked();
                    if (o->InternalFieldCount() > INSTANCE_OBJECT_JSOBJECT) {
                        o->SetAlignedPointerInInternalField(INSTANCE_OBJECT_JSOBJECT, nullptr);
                    }
                }
                const_cast<OpaqueJSContext *>(m_ctx)->MarkCollected(this);
                if (value) {
                    value->release();
                }
                weak.Reset();
            V8_UNLOCK()
        }

        Local<Value> L() const {
            if (value) {
                return value->Value();
            }
            return Local<Value>::New(m_ctx->Context()->isolate(), weak);
        }
        virtual void Clean(bool fromGC=false) const {
            if (m_count <= 0) {
                if (!HasFinalized()) {
                    const_cast<OpaqueJSValue *>(this)->m_finalized = true;
                    const JSClassDefinition *definition = m_fromClassDefinition;
                    while (definition) {
                        if (definition->finalize) {
                            definition->finalize(const_cast<JSObjectRef>(this));
                        }
                        definition =
                            definition->parentClass? definition->parentClass->Definition(): nullptr;
                    }
                }
                delete this;
            }
        }
        virtual int Retain() {
            if (!value) {
                V8_ISOLATE(m_ctx->Context()->Group(), isolate)
                    Context::Scope(m_ctx->Context()->Value());
                    value = JSValue<Value>::New(m_ctx->Context(), Local<Value>::New(isolate,weak));
                    weak.Reset();
                V8_UNLOCK()
            }
            return ++m_count;
        }
        virtual int Release(bool cleanOnZero=true) {
            int count = --m_count;
            ASSERT(count >= 0)
            if (cleanOnZero) {
                Clean();
            }
            return count;
        }
        virtual JSContextRef Context() const {
            return m_ctx;
        }
        virtual bool SetPrivateData(void *data) {
            if (m_fromClassDefinition) {
                m_private_data = data;
            }
            return m_fromClassDefinition;
        }
        virtual void *GetPrivateData() {
            return m_private_data;
        }
        virtual void SetFinalized() {
            m_finalized = true;
        }
        virtual bool HasFinalized() const {
            return m_finalized;
        }
        virtual bool IsClassObject() const {
            return m_fromClassDefinition;
        }

    protected:
        OpaqueJSValue(JSContextRef context, Local<Value> v, const JSClassDefinition* fromClass=0) :
            value(nullptr), m_ctx(context), m_fromClassDefinition(fromClass)
        {
            weak = UniquePersistent<Value>(context->Context()->isolate(), v);
            weak.SetWeak<OpaqueJSValue>(this, [](const WeakCallbackInfo<OpaqueJSValue> &info) {
                (info.GetParameter())->WeakCallback();
            }, v8::WeakCallbackType::kParameter);
            const_cast<OpaqueJSContext *>(context)->MarkForCollection(this);
        }

    private:
        virtual void WeakCallback() {
            weak.Reset();
        }

        JSValue<Value> *value;
        UniquePersistent<Value> weak;
        int m_count = 0;
        JSContextRef m_ctx;
        void *m_private_data = nullptr;
        const JSClassDefinition *m_fromClassDefinition = nullptr;
        bool m_finalized = false;
};

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
            ASSERT(m_value==nullptr)
            m_value = OpaqueJSValue::New(context,v);
            const_cast<OpaqueJSValue *>(m_value)->Retain();
            m_didSet = true;
        }
        void Set(JSContextRef context, const char *s) {
            ASSERT(m_value==nullptr)
            m_value = OpaqueJSValue::New(context,s);
            const_cast<OpaqueJSValue *>(m_value)->Retain();
            m_didSet = true;
        }
        void Set(JSValueRef v) {
            ASSERT(m_value==nullptr)
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
            ASSERT(false);
        }
        JSValueRef* operator&() { return &m_value; }

    private:
        JSValueRef *m_exceptionRef;
};


#endif //NODEDROID_JSC_H
