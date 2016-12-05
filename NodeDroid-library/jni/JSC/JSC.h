//
// Created by Eric on 11/6/16.
//

#ifndef NODEDROID_JSC_H
#define NODEDROID_JSC_H

#include "common.h"

#include <vector>
#include <string>

#define OpaqueJSValue                   JSValue<Value>
#define OpaqueJSContextGroup            ContextGroup
#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       JSValue<Array>

#include "JavaScriptCore/JavaScript.h"

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
        virtual JSValueRef InitInstance(JSContextRef ctx, Local<Object> instance, Local<Object> data);

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

#endif //NODEDROID_JSC_H
