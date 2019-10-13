/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_ObjectTemplate_h
#define V82JSC_ObjectTemplate_h

#include "Template.h"

namespace V82JSC {

struct ObjectTemplate : Template {
    JSValueRef m_named_data;
    JSValueRef m_indexed_data;
    JSValueRef m_access_check_data;
    JSValueRef m_failed_named_data;
    JSValueRef m_failed_indexed_data;
    v8::Persistent<v8::FunctionTemplate> m_constructor_template;
    v8::NamedPropertyHandlerConfiguration m_named_handler;
    v8::IndexedPropertyHandlerConfiguration m_indexed_handler;
    v8::AccessCheckCallback m_access_check;
    v8::NamedPropertyHandlerConfiguration m_named_failed_access_handler;
    v8::IndexedPropertyHandlerConfiguration m_indexed_failed_access_handler;
    bool m_need_proxy;
    int m_internal_fields;
    bool m_is_immutable_proto;
    
    static void Constructor(ObjectTemplate *obj) { Template::Constructor(obj); }
    static int Destructor(HeapContext& context, ObjectTemplate *obj)
    {
        int freed=0;
        freed += SmartReset<v8::FunctionTemplate>(context, obj->m_constructor_template);
        if (obj->m_named_data) JSValueUnprotect(obj->GetNullContext(), obj->m_named_data);
        if (obj->m_indexed_data) JSValueUnprotect(obj->GetNullContext(), obj->m_indexed_data);
        if (obj->m_access_check_data) JSValueUnprotect(obj->GetNullContext(), obj->m_access_check_data);
        if (obj->m_failed_named_data) JSValueUnprotect(obj->GetNullContext(), obj->m_failed_named_data);
        if (obj->m_failed_indexed_data) JSValueUnprotect(obj->GetNullContext(), obj->m_failed_indexed_data);
        return freed + Template::Destructor(context, obj);
    }

    v8::MaybeLocal<v8::Object> NewInstance(v8::Local<v8::Context> context, JSObjectRef root,
                                           bool isHiddenPrototype, JSClassDefinition* definition=nullptr,
                                           void* data=nullptr);
};

class DisableAccessChecksScope {
public:
    DisableAccessChecksScope(IsolateImpl* iso, ObjectTemplate* templ) :
    iso_(iso), templ_(templ), callback_(nullptr)
    {
        callback_ = templ_->m_access_check;
        templ_->m_access_check = nullptr;
    }
    ~DisableAccessChecksScope()
    {
        templ_->m_access_check = callback_;
    }
private:
    IsolateImpl *iso_;
    ObjectTemplate *templ_;
    v8::AccessCheckCallback callback_;
};

} /* namespace V82JSC */

#endif /* V82JSC_ObjectTemplate_h */
