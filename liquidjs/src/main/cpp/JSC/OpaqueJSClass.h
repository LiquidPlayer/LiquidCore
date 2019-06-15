/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_OPAQUEJSCLASS_H
#define LIQUIDCORE_OPAQUEJSCLASS_H

#include "JavaScriptCore/JavaScript.h"
#include "JSC/JSCRetainer.h"

struct OpaqueJSClass : public JSCRetainer {
    public:
        static JSClassRef New(const JSClassDefinition *definition);

        virtual ~OpaqueJSClass();
        inline const JSClassDefinition * Definition() { return m_definition; }

        void NewTemplate(Local<Context> context, Local<Value> *data,
            Local<ObjectTemplate> *templ);
        JSGlobalContextRef NewContext(JSContextGroupRef group);
        JSObjectRef InitInstance(JSContextRef ctx, Local<Object> instance,
            Local<Value> data, void *privateData);

        static void CallAsFunction(const FunctionCallbackInfo< Value > &);
        static void HasInstanceFunctionCallHandler(const FunctionCallbackInfo< Value > &);

    private:
        OpaqueJSClass(const JSClassDefinition *definition);
        static void StaticFunctionCallHandler(const FunctionCallbackInfo< Value > &);
        static void ConvertFunctionCallHandler(const FunctionCallbackInfo< Value > &);
        static void Finalize(const WeakCallbackInfo<UniquePersistent<Object>>&);

        static void NamedPropertyGetter(Local< Name >, const PropertyCallbackInfo< Value > &);
        static void NamedPropertyQuerier(Local< Name >, const PropertyCallbackInfo< Integer > &);
        static void NamedPropertySetter(Local<Name>,Local<Value>,
            const PropertyCallbackInfo<Value>&);
        static void NamedPropertyDeleter(Local< Name >, const PropertyCallbackInfo< Boolean > &);
        static void NamedPropertyEnumerator(const PropertyCallbackInfo< Array > &);

        static void IndexedPropertyGetter(uint32_t, const PropertyCallbackInfo< Value > &);
        static void IndexedPropertyQuerier(uint32_t, const PropertyCallbackInfo< Integer > &);
        static void IndexedPropertySetter(uint32_t,Local<Value>,const PropertyCallbackInfo<Value>&);
        static void IndexedPropertyDeleter(uint32_t, const PropertyCallbackInfo< Boolean > &);
        static void IndexedPropertyEnumerator(const PropertyCallbackInfo< Array > &);

        static void ProtoPropertyGetter(Local< Name >, const PropertyCallbackInfo< Value > &);
        static void ProtoPropertyQuerier(Local< Name >, const PropertyCallbackInfo< Integer > &);
        static void ProtoPropertyEnumerator(const PropertyCallbackInfo< Array > &);

        virtual bool IsFunction();
        virtual bool IsConstructor();
        const JSClassDefinition *m_definition;
};

#endif //LIQUIDCORE_OPAQUEJSCLASS_H
