/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Value_h
#define V82JSC_Value_h

#include "HeapObjects.h"
#include "Context.h"

namespace V82JSC {
    
struct Value : HeapObject {
    uint64_t reserved_; // For number, value is stored here
    JSValueRef m_value;
    uint64_t reserved2_; // For string, resource is stored here
    uint64_t reserved3_; // For string, resource data is stored here
    JSValueRef m_secondary_value;
    
    static void Constructor(Value *obj) {}
    static int Destructor(HeapContext& contet, Value *obj)
    {
        if (obj->m_value) JSValueUnprotect(obj->GetNullContext(), obj->m_value);
        if (obj->m_secondary_value) JSValueUnprotect(obj->GetNullContext(), obj->m_secondary_value);
        RemoveObjectFromMap(obj->GetIsolate(), (JSObjectRef)obj->m_value);
        return 0;
    }
    
    static void RemoveObjectFromMap(IsolateImpl* iso, JSObjectRef o);

    static v8::Local<v8::Value> New(const V82JSC::Context *ctx, JSValueRef value, V82JSC::BaseMap *map=nullptr);
};

struct WeakValue : Value {
    static void Constructor(WeakValue *obj)
    {
        Value::Constructor(obj);
    }
    static int Destructor(HeapContext& context, WeakValue *obj)
    {
        // Don't call value destructor
        return 0;
    }
};
    
} /* namepsace V82JSC */

#endif /* V82JSC_Value_h */
