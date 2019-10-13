/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_FunctionTemplate_h
#define V82JSC_FunctionTemplate_h

#include "Template.h"

namespace V82JSC {

struct FunctionTemplate : Template {
    v8::Persistent<v8::ObjectTemplate> m_instance_template;
    v8::Persistent<v8::FunctionTemplate> m_prototype_provider;
    v8::ConstructorBehavior m_behavior;
    v8::Persistent<v8::String> m_name;
    JSValueRef m_functions_map;
    int m_length;
    bool m_isHiddenPrototype;
    bool m_removePrototype;
    bool m_readOnlyPrototype;
    
    static void Constructor(FunctionTemplate *obj) { Template::Constructor(obj); }
    static int Destructor(HeapContext& context, FunctionTemplate *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::FunctionTemplate>(context,obj->m_prototype_provider);
        freed +=SmartReset<v8::ObjectTemplate>(context, obj->m_instance_template);
        freed +=SmartReset<v8::String>(context, obj->m_name);
        if (obj->m_functions_map) JSValueUnprotect(obj->GetNullContext(), obj->m_functions_map);
        return freed + Template::Destructor(context, obj);
    }

    static JSValueRef callAsConstructorCallback(JSContextRef ctx,
                                                JSObjectRef constructor,
                                                JSObjectRef thisObject,
                                                size_t argumentCount,
                                                const JSValueRef arguments[],
                                                JSValueRef* exception);
    static v8::MaybeLocal<v8::Function> GetFunction(v8::FunctionTemplate * templ,
                                                    v8::Local<v8::Context> context,
                                                    v8::Local<v8::Name> inferred_name);
};

} /* namespace V82JSC */

#endif /* V82JSC_FunctionTemplate_h */
