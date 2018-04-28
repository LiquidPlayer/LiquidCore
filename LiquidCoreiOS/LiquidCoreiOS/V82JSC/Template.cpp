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
    ValueImpl *value_ = V82JSC::ToImpl<ValueImpl>(value);
    
    Prop prop;
    prop.name = name_;
    prop.value = value_;
    prop.attributes = attributes;
    
    this_->m_properties.push_back(prop);
}
void Template::SetPrivate(Local<Private> name, Local<Data> value, PropertyAttribute attributes)
{
    assert(0);
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
    FunctionTemplateImpl *getter_ = V82JSC::ToImpl<FunctionTemplateImpl>(getter);
    FunctionTemplateImpl *setter_ = V82JSC::ToImpl<FunctionTemplateImpl>(setter);

    PropAccessor accessor;
    accessor.name = name_;
    accessor.getter = getter_;
    accessor.setter = setter_;
    accessor.attribute = attribute;
    accessor.settings = settings;
    
    this_->m_property_accessors.push_back(accessor);
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
    SetNativeDataProperty(name.As<Name>(),
                          reinterpret_cast<AccessorNameGetterCallback>(getter),
                          reinterpret_cast<AccessorNameSetterCallback>(setter),
                          data, attribute, signature, settings);
}

void Template::SetNativeDataProperty(
                           Local<Name> name, AccessorNameGetterCallback getter,
                           AccessorNameSetterCallback setter,
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
    assert(0);
}

/**
 * During template instantiation, sets the value with the intrinsic property
 * from the correct context.
 */
void Template::SetIntrinsicDataProperty(Local<Name> name, Intrinsic intrinsic,
                              PropertyAttribute attribute)
{
    assert(0);
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
    IsolateImpl* isolateimpl = wrap->m_isolate;
    Local<Context> context = ContextImpl::New(V82JSC::ToIsolate(isolateimpl), ctx);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    Local<Value> thiz = ValueImpl::New(ctximpl, thisObject);
    
    // Check signature
    bool signature_match = !wrap->m_template->m_signature;
    if (!signature_match) {
        InstanceWrap* thisWrap = V82JSC::getPrivateInstance(ctx, thisObject);
        SignatureImpl *sig = wrap->m_template->m_signature;
        if (thisWrap) {
            for (const TemplateImpl *templ = thisWrap->m_object_template->m_constructor_template;
                 !signature_match && templ; templ = templ->m_parent) {
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
    Local<Value> data = ValueImpl::New(ctximpl, wrap->m_template->m_data);
    
    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 0;
        O(isolateimpl),                                      // kIsolateIndex = 1;
        O(isolateimpl->i.roots.undefined_value),             // kReturnValueDefaultValueIndex = 2;
        O(isolateimpl->i.roots.undefined_value),             // kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        O(isolateimpl->i.roots.undefined_value),             // kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<Value> arg = ValueImpl::New(ctximpl, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    JSValueRef held_exception = isolateimpl->m_pending_exception;
    isolateimpl->m_pending_exception = 0;
    
    FunctionCallbackImpl info(implicit, values, (int) argumentCount);
    
    if (wrap->m_template->m_callback) {
        wrap->m_template->m_callback(info);
    }

    *exception = isolateimpl->m_pending_exception;
    isolateimpl->m_pending_exception = held_exception;
    
    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, context);
}

/** Creates a function template.*/
TemplateImpl* TemplateImpl::New(Isolate* isolate, size_t size)
{
    TemplateImpl *templ = (TemplateImpl *) malloc(size);
    memset(templ, 0, size);
    templ->pMap = (v8::internal::Map *)((reinterpret_cast<intptr_t>(&templ->map) & ~3) + 1);
    
    templ->m_properties = std::vector<Prop>();
    templ->m_property_accessors = std::vector<PropAccessor>();
    templ->m_accessors = std::vector<ObjAccessor>();
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
    Local<Object> thiz = ValueImpl::New(ctx, instance).As<Object>();
    
    for (auto i=m_properties.begin(); i!=m_properties.end(); ++i) {
        typedef internal::Object O;
        typedef internal::Internals I;
        O* obj = * reinterpret_cast<O* const *>(i->value);
        int t = I::GetInstanceType(obj);
        JSValueRef v = i->value->m_value;
        if (t == v8::internal::FUNCTION_TEMPLATE_INFO_TYPE) {
            MaybeLocal<Function> o = reinterpret_cast<FunctionTemplate*>(i->value)->GetFunction(context);
            if (!o.IsEmpty()) {
                v = V82JSC::ToJSValueRef<Function>(o.ToLocalChecked(), context);
            }
        } else if (t == v8::internal::OBJECT_TEMPLATE_INFO_TYPE) {
            MaybeLocal<Object> o = reinterpret_cast<ObjectTemplate*>(i->value)->NewInstance(context);
            if (!o.IsEmpty()) {
                v = V82JSC::ToJSValueRef<Object>(o.ToLocalChecked(), context);
            }
        }
        JSValueRef args[] = {
            instance,
            i->name->m_value,
            v,
            JSValueMakeNumber(ctx->m_ctxRef, i->attributes)
        };
        
        /* None = 0,
           ReadOnly = 1 << 0,
           DontEnum = 1 << 1,
           DontDelete = 1 << 2
        */
        V82JSC::exec(ctx->m_ctxRef,
                     "Object.defineProperty(_1, _2, "
                     "{ writable : !(_4&(1<<0)), "
                     "  enumerable : !(_4&(1<<1)), "
                     "  configurable : !(_4&(1<<2)), "
                     "  value: _3 })", 4, args, &exception);
        if (exception.ShouldThow()) return MaybeLocal<Object>();
    }
    
    for (auto i=m_property_accessors.begin(); i!=m_property_accessors.end(); ++i) {
        MaybeLocal<Function> getter = i->getter ? _local<FunctionTemplate>(i->getter).toLocal()->GetFunction(context) : MaybeLocal<Function>();
        if (i->getter && getter.IsEmpty()) return MaybeLocal<Object>();
        MaybeLocal<Function> setter = i->setter ? _local<FunctionTemplate>(i->setter).toLocal()->GetFunction(context) : MaybeLocal<Function>();
        if (i->setter && setter.IsEmpty()) return MaybeLocal<Object>();
        thiz->SetAccessorProperty(_local<Name>(i->name).toLocal(),
                                  getter.ToLocalChecked(),
                                  setter.ToLocalChecked(),
                                  i->attribute,
                                  i->settings);
    }
    
    for (auto i=m_accessors.begin(); i!=m_accessors.end(); ++i) {
        Maybe<bool> set = thiz->SetAccessor(context,
                                            _local<Name>(i->name).toLocal(),
                                            i->getter,
                                            i->setter,
                                            _local<Value>(i->data).toLocal(),
                                            i->settings,
                                            i->attribute);
        if (set.IsNothing()) return MaybeLocal<Object>();
    }
    
    return thiz;
}
