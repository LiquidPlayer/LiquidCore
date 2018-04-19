//
//  Template.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/**
 * Adds a property to each instance created by this template.
 *
 * The property must be defined either as a primitive value, or a template.
 */
void Template::Set(Local<Name> name, Local<Data> value, PropertyAttribute attributes)
{
    TemplateImpl *this_ = V82JSC::ToImpl<TemplateImpl,Template>(this);
    ValueImpl *name_ = V82JSC::ToImpl<ValueImpl>(name);
    JSStringRef s = JSValueToStringCopy(name_->m_context->m_context, name_->m_value, 0);
    ValueImpl *value_ = V82JSC::ToImpl<ValueImpl>(value);
    // FIXME: Deal with attributes
    for (auto i = this_->m_properties.begin(); i != this_->m_properties.end(); ++i ) {
        if (JSStringIsEqual(i->first, s)) {
            i->second = value_;
            JSStringRelease(s);
            return;
        }
    }
    this_->m_properties[s] = value_;
}
void Template::SetPrivate(Local<Private> name, Local<Data> value, PropertyAttribute attributes)
{
    
}

void Template::SetAccessorProperty(
                         Local<Name> name,
                         Local<FunctionTemplate> getter,
                         Local<FunctionTemplate> setter,
                         PropertyAttribute attribute,
                         AccessControl settings)
{
    TemplateImpl *this_ = V82JSC::ToImpl<TemplateImpl,Template>(this);
    ValueImpl *name_ = V82JSC::ToImpl<ValueImpl>(name);
    JSStringRef s = JSValueToStringCopy(name_->m_context->m_context, name_->m_value, 0);
    
    FunctionTemplateImpl *getter_ = V82JSC::ToImpl<FunctionTemplateImpl>(getter);
    FunctionTemplateImpl *setter_ = V82JSC::ToImpl<FunctionTemplateImpl>(setter);
    // FIXME: Deal with attributes
    // FIXME: Deal with AccessControl

    PropAccessor accessor;
    accessor.m_getter = getter_;
    accessor.m_setter = setter_;
    for (auto i = this_->m_property_accessors.begin(); i != this_->m_property_accessors.end(); ++i ) {
        if (JSStringIsEqual(i->first, s)) {
            i->second = accessor;
            JSStringRelease(s);
            return;
        }
    }
    this_->m_property_accessors[s] = accessor;
}

/**
 * Whenever the property with the given name is accessed on objects
 * created from this Template the getter and setter callbacks
 * are called instead of getting and setting the property directly
 * on the JavaScript object.
 *
 * \param name The name of the property for which an accessor is added.
 * \param getter The callback to invoke when getting the property.
 * \param setter The callback to invoke when setting the property.
 * \param data A piece of data that will be passed to the getter and setter
 *   callbacks whenever they are invoked.
 * \param settings Access control settings for the accessor. This is a bit
 *   field consisting of one of more of
 *   DEFAULT = 0, ALL_CAN_READ = 1, or ALL_CAN_WRITE = 2.
 *   The default is to not allow cross-context access.
 *   ALL_CAN_READ means that all cross-context reads are allowed.
 *   ALL_CAN_WRITE means that all cross-context writes are allowed.
 *   The combination ALL_CAN_READ | ALL_CAN_WRITE can be used to allow all
 *   cross-context access.
 * \param attribute The attributes of the property for which an accessor
 *   is added.
 * \param signature The signature describes valid receivers for the accessor
 *   and is used to perform implicit instance checks against them. If the
 *   receiver is incompatible (i.e. is not an instance of the constructor as
 *   defined by FunctionTemplate::HasInstance()), an implicit TypeError is
 *   thrown and no callback is invoked.
 */

void Template::SetNativeDataProperty(
                           Local<String> name, AccessorGetterCallback getter,
                           AccessorSetterCallback setter,
                           Local<Value> data, PropertyAttribute attribute,
                           Local<AccessorSignature> signature,
                           AccessControl settings)
{
    Local<ObjectTemplate> ot = _local<ObjectTemplate>(this).toLocal();
    ot->SetAccessor(name, getter, setter, data, settings, attribute, signature);
}

/**
 * Like SetNativeDataProperty, but V8 will replace the native data property
 * with a real data property on first access.
 */
void Template::SetLazyDataProperty(Local<Name> name, AccessorNameGetterCallback getter,
                         Local<Value> data,
                         PropertyAttribute attribute)
{
    
}

/**
 * During template instantiation, sets the value with the intrinsic property
 * from the correct context.
 */
void Template::SetIntrinsicDataProperty(Local<Name> name, Intrinsic intrinsic,
                              PropertyAttribute attribute)
{
    
}


#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

JSValueRef TemplateImpl::callAsFunctionCallback(JSContextRef ctx,
                                                JSObjectRef proxy_function,
                                                JSObjectRef thisObject,
                                                size_t argumentCount,
                                                const JSValueRef *arguments,
                                                JSValueRef *exception)
{
    TemplateWrap *wrap = reinterpret_cast<TemplateWrap*>(JSObjectGetPrivate(proxy_function));
    Local<Value> thiz = ValueImpl::New(wrap->m_context, thisObject);
    
    // Check signature
    JSStringRef sprivate = JSStringCreateWithUTF8CString("__private__");
    bool signature_match = !wrap->m_template->m_signature;
    if (!signature_match) {
        JSValueRef excp=0;
        JSObjectRef __private__ = (JSObjectRef) JSObjectGetProperty(ctx, thisObject, sprivate, &excp);
        assert(excp==0);
        SignatureImpl *sig = wrap->m_template->m_signature;
        if (JSValueIsObject(ctx, __private__)) {
            bool hasOwn = JSValueToBoolean(ctx,
               V82JSC::exec(ctx, "return !_1.__proto__ || (_1.__private__ !== _1.__proto__.__private__)", 1, &thisObject));
            InstanceWrap *thisWrap = reinterpret_cast<InstanceWrap*>(JSObjectGetPrivate(__private__));
            for (const TemplateImpl *templ = thisWrap->m_object_template->m_constructor_template;
                 !signature_match && hasOwn && templ; templ = templ->m_parent) {
                signature_match = sig->m_template == templ;
            }
        }
    }
    if (!signature_match) {
        JSStringRef message = JSStringCreateWithUTF8CString("new TypeError('Illegal invocation')");
        *exception = JSEvaluateScript(ctx, message, 0, 0, 0, 0);
        JSStringRelease(message);
        return 0;
    }
    Local<Value> data = ValueImpl::New(wrap->m_context, wrap->m_template->m_data);
    
    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 0;
        O(wrap->m_context->isolate),                         // kIsolateIndex = 1;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueDefaultValueIndex = 2;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<Value> arg = ValueImpl::New(wrap->m_context, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    FunctionCallbackImpl info(implicit, values, (int) argumentCount);
    
    if (wrap->m_template->m_callback) {
        wrap->m_template->m_callback(info);
    }
    
    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
}

/** Creates a function template.*/
TemplateImpl* TemplateImpl::New(Isolate* isolate, size_t size)
{
    TemplateImpl *templ = (TemplateImpl *) malloc(size);
    memset(templ, 0, size);
    templ->pMap = (v8::internal::Map *)((reinterpret_cast<intptr_t>(&templ->map) & ~3) + 1);
    
    templ->m_properties = std::map<JSStringRef, ValueImpl*>();
    templ->m_property_accessors = std::map<JSStringRef, PropAccessor>();
    templ->m_obj_accessors = std::map<JSStringRef, ObjAccessor>();
    templ->m_isolate = isolate;
    templ->m_definition = kJSClassDefinitionEmpty;
    templ->m_definition.attributes = kJSClassAttributeNoAutomaticPrototype;
    templ->m_parent = nullptr;

    return templ;
}

MaybeLocal<Object> TemplateImpl::InitInstance(Local<Context> context, JSObjectRef instance, LocalException& excep,
                                              const FunctionTemplateImpl *impl)
{
    Local<ObjectTemplate> instance_template =
        _local<FunctionTemplate>(const_cast<FunctionTemplateImpl*>(impl)).toLocal()->InstanceTemplate();
    ObjectTemplateImpl *instance_impl = V82JSC::ToImpl<ObjectTemplateImpl>(instance_template);
    
    MaybeLocal<Object> thiz;
    if (impl->m_parent) {
        thiz = InitInstance(context, instance, excep, impl->m_parent);
    }
    if (!excep.ShouldThow()) {
        thiz = instance_impl->InitInstance(context, instance, excep);
    }
    return thiz;
};

MaybeLocal<Object> TemplateImpl::InitInstance(Local<Context> context, JSObjectRef instance, LocalException& exception)
{
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    for (auto i=m_properties.begin(); i!=m_properties.end(); ++i) {
        typedef internal::Object O;
        typedef internal::Internals I;
        O* obj = reinterpret_cast<O* const>(i->second->pMap);
        int t = I::GetInstanceType(obj);
        JSValueRef v = i->second->m_value;
        if (t == v8::internal::FUNCTION_TEMPLATE_INFO_TYPE) {
            MaybeLocal<Function> o = reinterpret_cast<FunctionTemplate*>(i->second)->GetFunction(context);
            if (!o.IsEmpty()) {
                v = V82JSC::ToJSValueRef<Function>(o.ToLocalChecked(), context);
            }
        } else if (t == v8::internal::OBJECT_TEMPLATE_INFO_TYPE) {
            MaybeLocal<Object> o = reinterpret_cast<ObjectTemplate*>(i->second)->NewInstance(context);
            if (!o.IsEmpty()) {
                v = V82JSC::ToJSValueRef<Object>(o.ToLocalChecked(), context);
            }
        }
        JSObjectSetProperty(ctx->m_context, instance, i->first, v, kJSPropertyAttributeNone, &exception);
    }
    
    JSStringRef getset = JSStringCreateWithUTF8CString(
       "delete __o__[__n__]; "
       "if (!__setter__) Object.defineProperty(__o__, __n__, { get: __getter__, configurable: true }); "
       "else if (!__getter__) Object.defineProperty(__o__, __n__, { set: __setter__, configurable: true }); "
       "else Object.defineProperty(__o__, __n__, { get: __getter__, set: __setter__, configurable: true });");
    JSStringRef name = JSStringCreateWithUTF8CString("getset");
    JSStringRef paramNames[] = {
        JSStringCreateWithUTF8CString("__o__"),
        JSStringCreateWithUTF8CString("__n__"),
        JSStringCreateWithUTF8CString("__getter__"),
        JSStringCreateWithUTF8CString("__setter__"),
    };
    JSValueRef exp = nullptr;
    JSObjectRef getsetF = JSObjectMakeFunction(ctx->m_context,
                                               name,
                                               sizeof paramNames / sizeof (JSStringRef),
                                               paramNames,
                                               getset, 0, 0, &exception);
    assert(exp==nullptr);
    
    for (auto i=m_property_accessors.begin(); i!=m_property_accessors.end(); ++i) {
        Local<FunctionTemplate> getter = _local<FunctionTemplate>(i->second.m_getter).toLocal();
        Local<FunctionTemplate> setter = _local<FunctionTemplate>(i->second.m_setter).toLocal();
        if (getter.IsEmpty() && setter.IsEmpty()) continue;
        
        JSValueRef params[] = {
            instance,
            JSValueMakeString(ctx->m_context, i->first),
            0,
            0
        };
        if (!getter.IsEmpty()) {
            params[2] = V82JSC::ToJSValueRef<Function>(getter->GetFunction(context).ToLocalChecked(), context);
        }
        if (!setter.IsEmpty()) {
            params[3] = V82JSC::ToJSValueRef<Function>(setter->GetFunction(context).ToLocalChecked(), context);
        }
        JSObjectCallAsFunction(ctx->m_context,
                               getsetF,
                               0,
                               sizeof params / sizeof (JSValueRef),
                               params,
                               &exp);
        assert(exp==nullptr);
    }
    
    for (auto i=m_obj_accessors.begin(); i!=m_obj_accessors.end(); ++i) {
        ObjAccessor *priv = new ObjAccessor();
        priv->m_property = JSValueMakeString(ctx->m_context, i->first);
        JSValueProtect(ctx->m_context, priv->m_property);
        priv->m_data = i->second.m_data;
        JSValueProtect(ctx->m_context, priv->m_data);
        priv->m_setter = i->second.m_setter;
        priv->m_getter = i->second.m_getter;
        priv->m_context = ctx;
        
        JSObjectRef getter = 0;
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        if (priv->m_getter) {
            def.callAsFunction = TemplateImpl::objectGetterCallback;
            JSClassRef claz = JSClassCreate(&def);
            getter = JSObjectMake(ctx->m_context, claz, priv);
            JSClassRelease(claz);
        }
        JSObjectRef setter = 0;
        if (priv->m_setter) {
            def.callAsFunction = TemplateImpl::objectSetterCallback;
            JSClassRef claz = JSClassCreate(&def);
            setter = JSObjectMake(ctx->m_context, claz, priv);
            JSClassRelease(claz);
        }
        
        JSValueRef params[] = {
            instance,
            JSValueMakeString(ctx->m_context, i->first),
            getter,
            setter
        };
        JSObjectCallAsFunction(ctx->m_context,
                               getsetF,
                               0,
                               sizeof params / sizeof (JSValueRef),
                               params,
                               &exp);
        assert(exp==nullptr);
    }
    
    JSStringRelease(getset);
    JSStringRelease(name);
    for (int x=0; x<sizeof paramNames / sizeof(JSStringRef); x++) JSStringRelease(paramNames[x]);
    
    return _local<Object>(*ValueImpl::New(ctx, instance)).toLocal();
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

JSValueRef TemplateImpl::objectGetterCallback(JSContextRef ctx,
                                              JSObjectRef function,
                                              JSObjectRef thisObject,
                                              size_t argumentCount,
                                              const JSValueRef *arguments,
                                              JSValueRef *exception)
{
    ObjAccessor* wrap = reinterpret_cast<ObjAccessor*>(JSObjectGetPrivate(function));
    
    Local<Value> thiz = ValueImpl::New(wrap->m_context, thisObject);
    Local<Value> data = ValueImpl::New(wrap->m_context, wrap->m_data);
    
    v8::internal::Object * implicit[] = {
        0 /*FIXME*/,                                         // kShouldThrowOnErrorIndex = 0;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 1;
        O(wrap->m_context->isolate),                         // kIsolateIndex = 2;
        O(wrap->m_context->isolate->i.roots.the_hole_value), // kReturnValueDefaultValueIndex = 3;
        O(wrap->m_context->isolate->i.roots.the_hole_value), // kReturnValueIndex = 4;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 5;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kThisIndex = 6;
    };
    
    PropertyCallbackImpl<Value> info(implicit);
    
    JSValueRef held_exception = wrap->m_context->isolate->m_pending_exception;
    wrap->m_context->isolate->m_pending_exception = 0;

    wrap->m_getter(ValueImpl::New(wrap->m_context, wrap->m_property).As<String>(), info);

    *exception = wrap->m_context->isolate->m_pending_exception;
    wrap->m_context->isolate->m_pending_exception = held_exception;

    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
}

JSValueRef TemplateImpl::objectSetterCallback(JSContextRef ctx,
                                                    JSObjectRef function,
                                                    JSObjectRef thisObject,
                                                    size_t argumentCount,
                                                    const JSValueRef *arguments,
                                                    JSValueRef *exception)
{
    ObjAccessor* wrap = reinterpret_cast<ObjAccessor*>(JSObjectGetPrivate(function));
    
    Local<Value> thiz = ValueImpl::New(wrap->m_context, thisObject);
    Local<Value> data = ValueImpl::New(wrap->m_context, wrap->m_data);
    
    v8::internal::Object * implicit[] = {
        0 /*FIXME*/,                                         // kShouldThrowOnErrorIndex = 0;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 1;
        O(wrap->m_context->isolate),                         // kIsolateIndex = 2;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueDefaultValueIndex = 3;
        nullptr /*written by callee*/,                       // kReturnValueIndex = 4;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 5;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kThisIndex = 6;
    };
    assert(argumentCount==1);
    Local<Value> value = ValueImpl::New(wrap->m_context, arguments[0]);
    
    PropertyCallbackImpl<void> info(implicit);
    
    JSValueRef held_exception = wrap->m_context->isolate->m_pending_exception;
    wrap->m_context->isolate->m_pending_exception = 0;

    wrap->m_setter(ValueImpl::New(wrap->m_context, wrap->m_property).As<String>(), value, info);
    
    *exception = wrap->m_context->isolate->m_pending_exception;
    wrap->m_context->isolate->m_pending_exception = held_exception;
    
    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
}
