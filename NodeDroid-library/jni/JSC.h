//
// Created by Eric on 11/6/16.
//

#ifndef NODEDROID_JSC_H
#define NODEDROID_JSC_H

#include "JSJNI.h"

#define OpaqueJSValue                   JSValue<Value>
#define OpaqueJSContext                 JSContext
#define OpaqueJSContextGroup            ContextGroup
#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       JSValue<Array>

#include "JavaScriptCore/JavaScript.h"

class OpaqueJSString : public Retainer {
    public:
        OpaqueJSString(Isolate *isolate, Local<String> string);
        virtual ~OpaqueJSString();
        virtual Local<String> Value();
        virtual Isolate * isolate() { return m_isolate; }
        virtual const JSChar * Chars();

    private:
        Persistent<String, CopyablePersistentTraits<String>> m_value;
        Isolate *m_isolate;
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
