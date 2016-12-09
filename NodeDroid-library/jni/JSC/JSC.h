//
// Created by Eric on 11/6/16.
//

#ifndef NODEDROID_JSC_H
#define NODEDROID_JSC_H

#include "common.h"

#include <vector>
#include <string>

//#define OpaqueJSValue                   JSValue<Value>
#define OpaqueJSContextGroup            ContextGroup
#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       JSValue<Array>

#include "JavaScriptCore/JavaScript.h"

#define ASSERT(x) if(!(x)) \
    __android_log_assert("conditional", "ASSERT FAILED", "%s(%d) : %s", __FILE__, __LINE__, #x);

class OpaqueJSContext : public Retainer {
    public:
        OpaqueJSContext(JSContext *ctx);
        virtual ~OpaqueJSContext();
        JSContext *Context() const { return m_context; }

    private:
        JSContext *m_context;
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
        virtual JSObjectRef InitInstance(JSContextRef ctx, Local<Object> instance, Local<Object> data);

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
        OpaqueJSValue(JSContextRef context, Local<Value> v) :
            value(JSValue<Value>::New(context->Context(), v)){}
        OpaqueJSValue(JSContextRef context, const char *s) :
            value(JSValue<Value>::New(context->Context(),
                String::NewFromUtf8(context->Context()->isolate(),s)))
            {}
        virtual ~OpaqueJSValue() {
            value->release();
        }

        JSValue<Value> * operator->() const {
            return value;
        }
        virtual void Clean() const {
            if (m_count <= 0) {
                delete this;
            }
        }
        virtual int Retain() { return ++m_count; }
        virtual int Release(bool cleanOnZero=true) {
            int count = --m_count;
            ASSERT(count >= 0)
            if (cleanOnZero) {
                Clean();
            }
            return count;
        }

    private:
        JSValue<Value> *value;
        int m_count = 0;
};

class TempJSValue {
    public:
        TempJSValue() : m_value(nullptr) {}
        TempJSValue(JSContextRef context, Local<Value> v) : m_value(new OpaqueJSValue(context,v)) {
            const_cast<OpaqueJSValue *>(m_value)->Retain();
        }
        TempJSValue(JSContextRef context, const char *s) : m_value(new OpaqueJSValue(context,s)) {
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
            ASSERT(m_value==nullptr)
            m_value = new OpaqueJSValue(context,v);
            const_cast<OpaqueJSValue *>(m_value)->Retain();
            m_didSet = true;
        }
        void Set(JSContextRef context, const char *s) {
            ASSERT(m_value==nullptr)
            m_value = new OpaqueJSValue(context,s);
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
                const_cast<OpaqueJSValue *>(m_value)->Release(!m_exceptionRef);
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
