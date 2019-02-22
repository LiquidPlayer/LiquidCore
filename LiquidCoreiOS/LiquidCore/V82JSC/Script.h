/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Script_h
#define V82JSC_Script_h

#include "HeapObjects.h"

namespace V82JSC {

struct UnboundScript : HeapObject {
    JSScriptRef m_script;
    JSStringRef m_script_string;
    v8::Persistent<v8::Integer> m_id;
    v8::Persistent<v8::Value> m_resource_name;
    v8::Persistent<v8::Value> m_sourceURL;
    v8::Persistent<v8::Value> m_sourceMappingURL;
    v8::Persistent<v8::Integer> m_resource_line_offset;
    v8::Persistent<v8::Integer> m_resource_column_offset;
    bool m_resource_is_shared_cross_origin;
    bool m_resource_is_opaque;
    bool m_is_wasm;
    bool m_is_module;
    
    static void Constructor(UnboundScript *obj) {}
    static int Destructor(HeapContext& context, UnboundScript *obj)
    {
        if (obj->m_script) JSScriptRelease(obj->m_script);
        if (obj->m_script_string) JSStringRelease(obj->m_script_string);
        
        int freed=0;
        freed +=SmartReset<v8::Value>(context, obj->m_resource_name);
        freed +=SmartReset<v8::Value>(context, obj->m_sourceURL);
        freed +=SmartReset<v8::Value>(context, obj->m_sourceMappingURL);
        freed +=SmartReset<v8::Integer>(context, obj->m_resource_line_offset);
        freed +=SmartReset<v8::Integer>(context, obj->m_resource_column_offset);
        freed +=SmartReset<v8::Integer>(context, obj->m_id);
        return freed;
    }
};

struct Script : HeapObject {
    v8::Persistent<v8::UnboundScript> m_unbound_script;
    v8::Persistent<v8::Context> m_context;
    
    static void Constructor(Script *obj) {}
    static int Destructor(HeapContext& context, Script *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::UnboundScript>(context, obj->m_unbound_script);
        freed +=SmartReset<v8::Context>(context, obj->m_context);
        return freed;
    }
};

} /* namespace V82JSC */

#endif /* Script_h */
