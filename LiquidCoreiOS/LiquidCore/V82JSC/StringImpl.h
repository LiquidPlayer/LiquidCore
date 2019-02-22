/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_StringImpl_h
#define V82JSC_StringImpl_h

#include "Value.h"

namespace V82JSC {

struct String : Value {
    static void Constructor(String *obj)
    {
        Value::Constructor(obj);
    }
    static int Destructor(HeapContext& context, String *obj)
    {
        return Value::Destructor(context, obj);
    }

    static v8::Local<v8::String> New(
      v8::Isolate *isolate,
      JSStringRef string,
      V82JSC::BaseMap *map=nullptr,
      void *resource = nullptr,
      v8::NewStringType stringtype = v8::NewStringType::kNormal
    );
};

struct WeakExternalString : WeakValue {
    JSWeakRef m_weakRef;
    v8::String::ExternalStringResourceBase *m_resource;
    
    static void Constructor(WeakExternalString *obj)
    {
        Value::Constructor(obj);
    }
    static int Destructor(HeapContext& context, WeakExternalString *obj)
    {
        if (obj->m_weakRef) JSWeakRelease(obj->GetContextGroup(), obj->m_weakRef);
        obj->FinalizeExternalString();
        return WeakValue::Destructor(context, obj);
    }
    void FinalizeExternalString();

    static void Init(IsolateImpl* iso,
                     JSValueRef value,
                     v8::String::ExternalStringResourceBase *resource,
                     V82JSC::BaseMap* map);
};
    
} /* namespace V82JSC */

#endif /* V82SJC_StringImpl_h */
