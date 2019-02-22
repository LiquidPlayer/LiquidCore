/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Object_h
#define V82JSC_Object_h

#include "HeapObjects.h"

// These aren't really V8 values, but we want to use V8 handles to manage their
// lifecycle, so we pretend.
namespace v8 {
    struct TrackedObject : v8::Value {};
    struct AccessorInfo : v8::Value {};
};

namespace V82JSC {

struct TrackedObject : HeapObject {
    JSValueRef m_security;
    JSValueRef m_proxy_security;
    JSValueRef m_hidden_proxy_security;
    JSValueRef m_private_properties;
    JSObjectRef m_hidden_children_array;
    int m_num_internal_fields;
    JSObjectRef m_internal_fields_array;
    v8::Persistent<v8::ObjectTemplate> m_object_template;
    int m_hash;
    bool m_isHiddenPrototype;
    bool m_isGlobalObject;
    void *m_embedder_data[2];
    JSObjectRef m_access_control;
    JSObjectRef m_access_proxies;
    JSObjectRef m_global_object_access_proxies;
    bool m_isDetached;
    JSObjectRef m_reattached_global;
    JSObjectRef m_bound_function;
    
    struct {
        void *buffer;
        size_t byte_length;
        bool isExternal;
        bool isNeuterable;
        IsolateImpl *iso;
        v8::Persistent<v8::TrackedObject> m_self;
    } ArrayBufferInfo;
    
    static void Constructor(TrackedObject *obj) {}
    static int Destructor(HeapContext& context, TrackedObject *obj)
    {
        int freed=0;
        freed += SmartReset<v8::ObjectTemplate>(context, obj->m_object_template);
        // obj->m_security is a weak reference to avoid circular referencing
        if (obj->m_proxy_security) JSValueUnprotect(obj->GetNullContext(), obj->m_proxy_security);
        if (obj->m_hidden_proxy_security) JSValueUnprotect(obj->GetNullContext(), obj->m_hidden_proxy_security);
        if (obj->m_private_properties) JSValueUnprotect(obj->GetNullContext(), obj->m_private_properties);
        if (obj->m_hidden_children_array) JSValueUnprotect(obj->GetNullContext(), obj->m_hidden_children_array);
        if (obj->m_internal_fields_array) JSValueUnprotect(obj->GetNullContext(), obj->m_internal_fields_array);
        if (obj->m_access_control) JSValueUnprotect(obj->GetNullContext(), obj->m_access_control);
        if (obj->m_access_proxies) JSValueUnprotect(obj->GetNullContext(), obj->m_access_proxies);
        if (obj->m_global_object_access_proxies) JSValueUnprotect(obj->GetNullContext(), obj->m_global_object_access_proxies);
        if (obj->m_reattached_global) JSValueUnprotect(obj->GetNullContext(), obj->m_reattached_global);
        if (obj->m_bound_function) JSValueUnprotect(obj->GetNullContext(), obj->m_bound_function);
        return freed;
    }
    
    static v8::Local<v8::Value> SecureValue(v8::Local<v8::Value> in_,
                                            v8::Local<v8::Context> ctx = v8::Local<v8::Context>());
    static TrackedObject* makePrivateInstance(IsolateImpl* iso, JSContextRef ctx, JSObjectRef object);
    static TrackedObject* getPrivateInstance(JSContextRef ctx, JSObjectRef object);
    static TrackedObject* makePrivateInstance(IsolateImpl* iso, JSContextRef ctx);
    static void setPrivateInstance(IsolateImpl* iso, JSContextRef ctx,
                                   TrackedObject* impl, JSObjectRef object);
};

struct Accessor : HeapObject {
    JSValueRef m_property;
    JSValueRef m_data;
    JSValueRef m_holder;
    v8::Persistent<v8::Signature> signature;
    v8::AccessorNameGetterCallback getter;
    v8::AccessorNameSetterCallback setter;
    
    static void Constructor(Accessor *obj) {}
    static int Destructor(HeapContext& context, Accessor *obj)
    {
        if (obj->m_property) JSValueUnprotect(obj->GetNullContext(), obj->m_property);
        if (obj->m_data) JSValueUnprotect(obj->GetNullContext(), obj->m_data);
        if (obj->m_holder) JSValueUnprotect(obj->GetNullContext(), obj->m_holder);
        return SmartReset<v8::Signature>(context, obj->signature);
    }
};

struct ObjectImpl : v8::Object {
    v8::Maybe<bool> SetAccessor(v8::Local<v8::Context> context,
                                v8::Local<v8::Name> name,
                                v8::AccessorNameGetterCallback getter,
                                v8::AccessorNameSetterCallback setter,
                                v8::MaybeLocal<v8::Value> data,
                                v8::AccessControl settings,
                                v8::PropertyAttribute attribute,
                                v8::Local<v8::Signature> signature);
};

struct HiddenObject : Value {
    void PropagateOwnPropertyToChild(v8::Local<v8::Context> context, v8::Local<v8::Name> property, JSObjectRef child);
    void PropagateOwnPropertyToChildren(v8::Local<v8::Context> context, v8::Local<v8::Name> property);
    void PropagateOwnPropertiesToChild(v8::Local<v8::Context> context, JSObjectRef child);
};
    
template <class T>
struct PropertyCallback : public v8::PropertyCallbackInfo<T>
{
    inline PropertyCallback(v8::internal::Object** args) :
    v8::PropertyCallbackInfo<T>(args) {}
};

} /* namespace V82JSC */

#endif /* V82JSC_Object_h */
