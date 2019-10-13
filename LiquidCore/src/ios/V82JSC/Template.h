/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Template_h
#define V82JSC_Template_h

#include "HeapObjects.h"

// These aren't really V8 values, but we want to use V8 handles to manage their
// lifecycle, so we pretend.
namespace v8 {
    struct PropAccessor : v8::Value {};
    struct Prop : v8::Value {};
    struct ObjAccessor : v8::Value {};
    struct IntrinsicProp : v8::Value {};
};

namespace V82JSC {

struct LocalException;

struct PropAccessor : HeapObject {
    v8::Persistent<v8::Name> name;
    v8::Persistent<v8::FunctionTemplate> setter;
    v8::Persistent<v8::FunctionTemplate> getter;
    v8::PropertyAttribute attribute;
    v8::AccessControl settings;
    v8::Persistent<v8::PropAccessor> next_;
    
    static void Constructor(PropAccessor *obj) {}
    static int Destructor(HeapContext& context, PropAccessor *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::Name>(context, obj->name);
        freed +=SmartReset<v8::FunctionTemplate>(context, obj->setter);
        freed +=SmartReset<v8::FunctionTemplate>(context, obj->getter);
        freed +=SmartReset<v8::PropAccessor>(context, obj->next_);
        return freed;
    }
};

struct Prop : HeapObject {
    v8::Persistent<v8::Name> name;
    v8::Persistent<v8::Data> value;
    v8::PropertyAttribute attributes;
    v8::Persistent<v8::Prop> next_;
    
    static void Constructor(Prop *obj) {}
    static int Destructor(HeapContext& context, Prop *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::Name>(context, obj->name);
        freed +=SmartReset<v8::Data>(context, obj->value);
        freed +=SmartReset<v8::Prop>(context, obj->next_);
        return freed;
    }
};

struct ObjAccessor : HeapObject {
    v8::Persistent<v8::Name> name;
    v8::Persistent<v8::Value> data;
    v8::Persistent<v8::Signature> signature;
    v8::AccessorNameGetterCallback getter;
    v8::AccessorNameSetterCallback setter;
    v8::AccessControl settings;
    v8::PropertyAttribute attribute;
    v8::Persistent<v8::ObjAccessor> next_;
    
    static void Constructor(ObjAccessor *obj) {}
    static int Destructor(HeapContext& context, ObjAccessor *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::Name>(context, obj->name);
        freed +=SmartReset<v8::Value>(context, obj->data);
        freed +=SmartReset<v8::Signature>(context, obj->signature);
        freed +=SmartReset<v8::ObjAccessor>(context, obj->next_);
        return freed;
    }
};

struct IntrinsicProp : HeapObject {
    v8::Persistent<v8::Name> name;
    v8::Intrinsic value;
    v8::Persistent<v8::IntrinsicProp> next_;
    
    static void Constructor(IntrinsicProp *obj) {}
    static int Destructor(HeapContext& context, IntrinsicProp *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::Name>(context, obj->name);
        freed +=SmartReset<v8::IntrinsicProp>(context, obj->next_);
        return freed;
    }
};

struct Signature : HeapObject {
    v8::Persistent<v8::FunctionTemplate> m_template;
    
    static void Constructor(Signature *obj) {}
    static int Destructor(HeapContext& context, Signature *obj)
    {
        return SmartReset<v8::FunctionTemplate>(context, obj->m_template);
    }
};

struct Template : HeapObject {
    JSValueRef m_data;
    v8::Persistent<v8::Prop> m_properties;
    v8::Persistent<v8::PropAccessor> m_property_accessors;
    v8::Persistent<v8::ObjAccessor> m_accessors;
    v8::Persistent<v8::IntrinsicProp> m_intrinsics;
    v8::Persistent<v8::Signature> m_signature;
    v8::Persistent<v8::ObjectTemplate> m_prototype_template;
    v8::Persistent<v8::FunctionTemplate> m_parent;
    v8::FunctionCallback m_callback;
    
    static void Constructor(Template *obj) {}
    static int Destructor(HeapContext& context, Template *obj)
    {
        int freed=0;
        freed +=SmartReset<v8::Prop>(context, obj->m_properties);
        freed +=SmartReset<v8::PropAccessor>(context, obj->m_property_accessors);
        freed +=SmartReset<v8::ObjAccessor>(context, obj->m_accessors);
        freed +=SmartReset<v8::IntrinsicProp>(context, obj->m_intrinsics);
        freed +=SmartReset<v8::Signature>(context, obj->m_signature);
        freed +=SmartReset<v8::ObjectTemplate>(context, obj->m_prototype_template);
        freed +=SmartReset<v8::FunctionTemplate>(context, obj->m_parent);
        return freed;
    }
    
    static JSValueRef callAsFunctionCallback(JSContextRef ctx,
                                             JSObjectRef function,
                                             JSObjectRef thisObject,
                                             size_t argumentCount,
                                             const JSValueRef arguments[],
                                             JSValueRef* exception);
    static JSObjectRef callAsConstructorCallback(JSContextRef ctx,
                                                 JSObjectRef constructor,
                                                 size_t argumentCount,
                                                 const JSValueRef arguments[],
                                                 JSValueRef* exception);
    static v8::MaybeLocal<v8::Object> InitInstance(v8::Local<v8::Context> context,
                                                   JSObjectRef instance,
                                                   LocalException& excep,
                                                   v8::Local<v8::FunctionTemplate> ftempl);
    v8::MaybeLocal<v8::Object> InitInstance(v8::Local<v8::Context> context,
                                            JSObjectRef instance, LocalException& exception);
    static Template* New(v8::Isolate* isolate, size_t size);
};

struct FunctionCallback : public v8::FunctionCallbackInfo<v8::Value>
{
    inline FunctionCallback(v8::internal::Object** implicit_args,
                            v8::internal::Object** values, int length) :
    FunctionCallbackInfo<v8::Value>(implicit_args, values, length) {}
};

} /* namespace V82JSC */

#endif /* V82JSC_Template_h */
