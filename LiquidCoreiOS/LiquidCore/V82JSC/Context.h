/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Context_h
#define V82JSC_Context_h

#include "HeapObjects.h"

// These aren't really V8 values, but we want to use V8 handles to manage their
// lifecycle, so we pretend.
namespace v8 {
    struct EmbeddedFixedArray : v8::Value {};
};

namespace V82JSC {

struct FixedArray : HeapObject {
    union {
        uint8_t __buffer[v8::internal::Internals::kFixedArrayHeaderSize - sizeof(HeapObject)];
        int m_size;
    };
    v8::internal::Object* m_elements[0];
    
    static void Constructor(FixedArray *obj) {}
    static int Destructor(HeapContext& context, FixedArray *obj)
    {
        int freed=0;
        for (int i=0; i<obj->m_size; i++) {
            if (obj->m_elements[i]->IsHeapObject()) {
                freed += DecrementCount(context, obj->m_elements[i]);
            }
        }
        return freed;
    }
};

struct Context : HeapObject {
    uint8_t reserved_[v8::internal::Internals::kContextHeaderSize +
                      ((v8::internal::kApiPointerSize + 1) * v8::internal::Internals::kContextEmbedderDataIndex)
                      - sizeof(HeapObject)];
    JSContextRef m_ctxRef;
    
    static void Constructor(Context *obj) {}
    static int Destructor(HeapContext& context, Context *obj)
    {
        return 0;
    }
};

struct GlobalContext : Context {
    v8::Persistent<v8::EmbeddedFixedArray> m_embedder_data;
    v8::Persistent<v8::Function> ObjectSetPrototypeOf;
    v8::Persistent<v8::Function> ObjectGetPrototypeOf;
    v8::Persistent<v8::Function> ObjectPrototypeToString;
    v8::Persistent<v8::Function> FunctionPrototypeBind;
    v8::Persistent<v8::Function> Eval;
    v8::Persistent<v8::String> m_code_gen_error;
    v8::Persistent<v8::Value> m_security_token;
    bool m_code_eval_from_strings_disallowed;
    JSObjectRef m_creation_context;
    JSValueRef m_proxy_targets;

    static void Constructor(GlobalContext *obj) {}
    static int Destructor(HeapContext& context, GlobalContext *obj)
    {
        IsolateImpl *iso = obj->GetIsolate();
        
        if (obj->m_creation_context) JSValueUnprotect(obj->GetNullContext(), obj->m_creation_context);
        if (obj->m_proxy_targets) JSValueUnprotect(obj->GetNullContext(), obj->m_proxy_targets);
        if (obj->m_ctxRef) JSGlobalContextRelease((JSGlobalContextRef)obj->m_ctxRef);
        int freed=0;
        freed +=SmartReset<v8::Function>(context, obj->ObjectSetPrototypeOf);
        freed +=SmartReset<v8::Function>(context, obj->ObjectGetPrototypeOf);
        freed +=SmartReset<v8::Function>(context, obj->ObjectPrototypeToString);
        freed +=SmartReset<v8::Function>(context, obj->FunctionPrototypeBind);
        freed +=SmartReset<v8::Function>(context, obj->Eval);
        freed +=SmartReset<v8::String>(context, obj->m_code_gen_error);
        freed +=SmartReset<v8::EmbeddedFixedArray>(context, obj->m_embedder_data);
        freed +=SmartReset<v8::Value>(context, obj->m_security_token);
        
        RemoveContextFromIsolate(iso, (JSGlobalContextRef)obj->m_ctxRef);
        
        return freed + Context::Destructor(context, obj);
    }
    static void RemoveContextFromIsolate(IsolateImpl* iso, JSGlobalContextRef ctx);
};

struct LocalContext : Context
{
    static v8::Local<v8::Context> New(v8::Isolate *isolate, JSContextRef ctx);
};
    

} /* namespace V82JSC */

#endif /* V82JSC_Context_h */
