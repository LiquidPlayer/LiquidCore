/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_OBJECTDATA_H
#define LIQUIDCORE_OBJECTDATA_H

#include "JavaScriptCore/JavaScript.h"
#include "JSC/JSCRetainer.h"

class ObjectData {
    public:
        static Local<Value> New(const JSClassDefinition *def = nullptr, JSContextRef ctx = nullptr,
                               JSClassRef cls = nullptr);
        static ObjectData* Get(Local<Value> value);
        void SetContext(JSContextRef ctx);
        void SetName(Local<Value> name);
        void SetFunc(Local<Object> func);
        inline const JSClassDefinition *Definition() { return m_definition; }
        inline JSContextRef       Context() { return m_context; }
        inline JSClassRef         Class() { return m_class; }
        inline const char *       Name() { return m_name; }
        inline Local<Object>      Func() { return Local<Object>::New(Isolate::GetCurrent(), m_func); }

    protected:
        ~ObjectData();

    private:
        ObjectData(const JSClassDefinition *def, JSContextRef ctx, JSClassRef cls);

        const JSClassDefinition*m_definition;
        JSContextRef            m_context;
        JSClassRef              m_class;
        UniquePersistent<Value> m_weak;
        char *                  m_name;
        Persistent<Object, CopyablePersistentTraits<Object>> m_func;
};

#endif //LIQUIDCORE_OBJECTDATA_H
