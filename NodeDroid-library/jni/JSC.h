//
// Created by Eric on 11/6/16.
//

#ifndef NODEDROID_JSC_H
#define NODEDROID_JSC_H

#include "JSJNI.h"

#include <vector>
#include <string>

#define OpaqueJSValue                   JSValue<Value>
#define OpaqueJSContext                 JSContext
#define OpaqueJSContextGroup            ContextGroup
#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       JSValue<Array>

#include "JavaScriptCore/JavaScript.h"

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
};

class OpaqueJSClass : public Retainer {
    public:
        OpaqueJSClass(const JSClassDefinition *definition);
        virtual ~OpaqueJSClass();

        virtual JSObjectRef NewInstance(JSContextRef);

    static void StaticAccessorGetter(Local< String >, const PropertyCallbackInfo< Value > &);
    static void StaticAccessorSetter(Local<String>, Local<Value>,const PropertyCallbackInfo<void>&);
    static void StaticFunctionAccessorGetter(Local< String >, const PropertyCallbackInfo< Value >&);
    static void StaticFunctionCallHandler(const FunctionCallbackInfo< Value > &);
    static void NamedPropertyGetter(Local< String >, const PropertyCallbackInfo< Value > &);
    static void NamedPropertySetter(Local<String>,Local<Value>, const PropertyCallbackInfo<Value>&);
    static void NamedPropertyDeleter(Local< String >, const PropertyCallbackInfo< Boolean > &);
    static void NamedPropertyEnumerator(const PropertyCallbackInfo< Array > &);
    static void CallAsFunction(const FunctionCallbackInfo< Value > &);
    static void Finalize(const WeakCallbackInfo<UniquePersistent<Object>>&);

    private:
        const JSClassDefinition *m_definition;

};

#endif //NODEDROID_JSC_H
