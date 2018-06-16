//
//  Template.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;
#define H V82JSC_HeapObject

/**
 * Adds a property to each instance created by this template.
 *
 * The property must be defined either as a primitive value, or a template.
 */
void Template::Set(Local<Name> name, Local<Data> value, PropertyAttribute attributes)
{
    TemplateImpl *this_ = V82JSC::ToImpl<TemplateImpl,Template>(this);
    Isolate *isolate = V82JSC::ToIsolate(this);
    HandleScope scope(isolate);

    H::Prop *prop = static_cast<H::Prop *>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(this_), V82JSC::ToIsolateImpl(this_)->m_property_accessor_map));

    prop->name.Reset(isolate, name);
    prop->value.Reset(isolate, value);
    prop->attributes = attributes;
    prop->next_.Reset(isolate, this_->m_properties.Get(isolate));
    Local<v8::Prop> local = V82JSC::CreateLocal<v8::Prop>(isolate, prop);
    this_->m_properties.Reset(isolate, local);
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
    Isolate *isolate = V82JSC::ToIsolate(this);

    H::PropAccessor *accessor = static_cast<H::PropAccessor *>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(this_), V82JSC::ToIsolateImpl(this_)->m_property_accessor_map));

    accessor->name.Reset(isolate, name);
    accessor->getter.Reset(isolate, getter);
    accessor->setter.Reset(isolate, setter);
    accessor->attribute = attribute;
    accessor->settings = settings;
    accessor->next_.Reset(isolate, this_->m_property_accessors.Get(isolate));
    Local<v8::PropAccessor> local = V82JSC::CreateLocal<v8::PropAccessor>(isolate, accessor);
    this_->m_property_accessors.Reset(isolate, local);
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
    ObjectTemplate *ot = reinterpret_cast<ObjectTemplate*>(this);
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
    TemplateImpl *templ = V82JSC::ToImpl<TemplateImpl, Template>(this);
    Isolate *isolate = V82JSC::ToIsolate(this);
    
    H::IntrinsicProp *prop = static_cast<H::IntrinsicProp *>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate), V82JSC::ToIsolateImpl(isolate)->m_intrinsic_property_map));
    
    prop->next_.Reset(isolate, templ->m_intrinsics.Get(isolate));
    Local<v8::IntrinsicProp> local = V82JSC::CreateLocal<v8::IntrinsicProp>(isolate, prop);
    templ->m_intrinsics.Reset(isolate, local);

    prop->name.Reset(isolate, name);
    prop->value = intrinsic;
}


#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

template<typename T>
static T callAsCallback(JSContextRef ctx,
                        JSObjectRef proxy_function,
                        JSObjectRef thisObject,
                        size_t argumentCount,
                        const JSValueRef *arguments,
                        JSValueRef *exception)
{
    bool isConstructCall = std::is_same<T, JSObjectRef>::value;
    IsolateImpl *iso = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
    Isolate *isolate = V82JSC::ToIsolate(iso);
    HandleScope scope(isolate);
    Local<v8::Template> local = V82JSC::FromPersistentData<v8::Template>(isolate, JSObjectGetPrivate(proxy_function));
    
    Local<Context> context = LocalContextImpl::New(isolate, ctx);
    Context::Scope context_scope(context);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    TemplateImpl *templ = V82JSC::ToImpl<TemplateImpl>(local);
    
    if (templ->m_map->instance_type() == internal::OBJECT_TEMPLATE_INFO_TYPE) {
        thisObject = proxy_function;
    }
    Local<Value> thiz = ValueImpl::New(ctximpl, thisObject);
    Local<Value> holder = thiz;

    // Check signature
    bool signature_match = templ->m_signature.IsEmpty();
    if (!signature_match) {
        SignatureImpl *sig = V82JSC::ToImpl<SignatureImpl>(templ->m_signature.Get(isolate));
        const TemplateImpl *sig_templ = V82JSC::ToImpl<TemplateImpl>(sig->m_template.Get(isolate));
        JSValueRef proto = thisObject;
        TrackedObjectImpl* thisWrap = getPrivateInstance(ctx, (JSObjectRef)proto);
        while(!signature_match && JSValueIsObject(ctx, proto)) {
            if (thisWrap && !thisWrap->m_object_template.IsEmpty() && (proto == thisObject || thisWrap->m_isHiddenPrototype)) {
                holder = proto == thisObject? thiz : ValueImpl::New(ctximpl, proto);
                ObjectTemplateImpl* ot = V82JSC::ToImpl<ObjectTemplateImpl>(thisWrap->m_object_template.Get(isolate));
                Local<FunctionTemplate> ctort = ot->m_constructor_template.Get(isolate);
                const TemplateImpl *templ = ctort.IsEmpty() ? nullptr : V82JSC::ToImpl<TemplateImpl>(ctort);
                while (!signature_match && templ) {
                    signature_match = sig_templ == templ;
                    templ = templ->m_parent.IsEmpty() ? nullptr : V82JSC::ToImpl<TemplateImpl>(templ->m_parent.Get(isolate));
                }
            }
            proto = V82JSC::GetRealPrototype(context, (JSObjectRef)proto);
            thisWrap = getPrivateInstance(ctx, (JSObjectRef)proto);
            if (!thisWrap || !thisWrap->m_isHiddenPrototype) break;
        }
    }
    if (!signature_match) {
        JSStringRef message = JSStringCreateWithUTF8CString("new TypeError('Illegal invocation')");
        *exception = JSEvaluateScript(ctx, message, 0, 0, 0, 0);
        JSStringRelease(message);
        return 0;
    }
    
    ++ iso->m_callback_depth;

    Local<Value> data = ValueImpl::New(ctximpl, templ->m_data);
    typedef v8::internal::Heap::RootListIndex R;
    internal::Object *the_hole = iso->ii.heap()->root(R::kTheHoleValueRootIndex);
    internal::Object *target = iso->ii.heap()->root(R::kUndefinedValueRootIndex);
    
    if (isConstructCall) {
        JSValueRef t = JSObjectMake(ctx, 0, 0);
        target = * reinterpret_cast<v8::internal::Object**> (*ValueImpl::New(ctximpl, t));
    }

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*holder), // kHolderIndex = 0;
        O(iso),                                              // kIsolateIndex = 1;
        the_hole,                                            // kReturnValueDefaultValueIndex = 2;
        the_hole,                                            // kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        target,                                              // kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<Value> arg = ValueImpl::New(ctximpl, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    iso->ii.thread_local_top()->scheduled_exception_ = the_hole;
    
    FunctionCallbackImpl info(implicit, values, (int) argumentCount);
    TryCatch try_catch(V82JSC::ToIsolate(iso));
    
    if (templ->m_callback) {
        templ->m_callback(info);
    } else {
        info.GetReturnValue().Set(info.This());
    }

    if (try_catch.HasCaught()) {
        *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
    } else if (iso->ii.thread_local_top()->scheduled_exception_ != the_hole) {
        internal::Object * excep = iso->ii.thread_local_top()->scheduled_exception_;
        *exception = V82JSC::ToJSValueRef_<Value>(excep, context);
        iso->ii.thread_local_top()->scheduled_exception_ = the_hole;
    }

    -- iso->m_callback_depth;
    
    if (!*exception) {
        Local<Value> ret = info.GetReturnValue().Get();

        return (T) V82JSC::ToJSValueRef<Value>(ret, context);
    } else {
        return NULL;
    }
}

JSValueRef TemplateImpl::callAsFunctionCallback(JSContextRef ctx,
                                                JSObjectRef proxy_function,
                                                JSObjectRef thisObject,
                                                size_t argumentCount,
                                                const JSValueRef *arguments,
                                                JSValueRef *exception)
{
    return callAsCallback<JSValueRef>(ctx, proxy_function, thisObject, argumentCount, arguments, exception);
}

JSObjectRef TemplateImpl::callAsConstructorCallback(JSContextRef ctx,
                                                    JSObjectRef constructor,
                                                    size_t argumentCount,
                                                    const JSValueRef *arguments,
                                                    JSValueRef *exception)
{
    return callAsCallback<JSObjectRef>(ctx, constructor, 0, argumentCount, arguments, exception);
}

MaybeLocal<Object> TemplateImpl::InitInstance(Local<Context> context, JSObjectRef instance, LocalException& excep,
                                              Local<FunctionTemplate> ft)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl>(ft);
    Isolate *isolate = V82JSC::ToIsolate(impl);
    Local<ObjectTemplate> instance_template = ft->InstanceTemplate();
    ObjectTemplateImpl *instance_impl = V82JSC::ToImpl<ObjectTemplateImpl>(instance_template);
    
    MaybeLocal<Object> thiz;
    if (!impl->m_parent.IsEmpty()) {
        thiz = InitInstance(context, instance, excep, impl->m_parent.Get(isolate));
    }
    if (!excep.ShouldThow()) {
        thiz = reinterpret_cast<TemplateImpl*>(instance_impl)->InitInstance(context, instance, excep);
    }
    return thiz;
};

MaybeLocal<Object> TemplateImpl::InitInstance(Local<Context> context, JSObjectRef instance, LocalException& exception)
{
    const ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctximpl);
    Isolate *isolate = V82JSC::ToIsolate(iso);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    Local<Object> thiz = ValueImpl::New(ctximpl, instance).As<Object>();
    TrackedObjectImpl *wrap = getPrivateInstance(ctx, instance);
    
    Local<Object> real_properties = Object::New(isolate);

    // Reverse lists so last added properties take precendence
    std::vector<PropImpl*> props;
    for (auto i=m_properties.Get(isolate); !i.IsEmpty(); ) {
        PropImpl *prop = V82JSC::ToImpl<PropImpl>(i);
        props.push_back(prop);
        i = prop->next_.Get(isolate);
    }
    for (auto i=props.rbegin(); i!=props.rend(); ++i) {
        PropImpl *prop = (*i);
        typedef internal::Object O;
        typedef internal::Internals I;
        Local<Data> vv = prop->value.Get(isolate);
        O* obj = * reinterpret_cast<O* const *>(*vv);
        JSValueRef v = V82JSC::ToJSValueRef(vv, context);
        if (obj->IsHeapObject()) {
            int t = I::GetInstanceType(obj);
            if (t == v8::internal::FUNCTION_TEMPLATE_INFO_TYPE) {
                FunctionTemplateImpl *fti = V82JSC::ToImpl<FunctionTemplateImpl>(vv);
                Local<FunctionTemplate> ft = V82JSC::CreateLocal<FunctionTemplate>(isolate, fti);
                MaybeLocal<Function> o = FunctionTemplateImpl::GetFunction(*ft, context, prop->name.Get(isolate));
                if (!o.IsEmpty()) {
                    v = V82JSC::ToJSValueRef<Function>(o.ToLocalChecked(), context);
                }
            } else if (t == v8::internal::OBJECT_TEMPLATE_INFO_TYPE) {
                ObjectTemplateImpl *oti = V82JSC::ToImpl<ObjectTemplateImpl>(vv);
                Local<ObjectTemplate> ot = V82JSC::CreateLocal<ObjectTemplate>(isolate, oti);
                MaybeLocal<Object> o = ot->NewInstance(context);
                if (!o.IsEmpty()) {
                    v = V82JSC::ToJSValueRef<Object>(o.ToLocalChecked(), context);
                }
            }
        }
        JSValueRef args[] = {
            instance,
            V82JSC::ToJSValueRef(prop->name.Get(isolate), context),
            v,
            JSValueMakeNumber(ctx, prop->attributes),
            V82JSC::ToJSValueRef(real_properties, context)
        };
        
        /* None = 0,
           ReadOnly = 1 << 0,
           DontEnum = 1 << 1,
           DontDelete = 1 << 2
        */
        
        if (wrap->m_isHiddenPrototype) {
            V82JSC::exec(ctx,
                         "_5[_2] = _3; "
                         "Object.defineProperty(_1, _2, "
                         "{ enumerable : !(_4&(1<<1)), "
                         "  configurable : !(_4&(1<<2)), "
                         "  set(v) { return _5[_2] = v; }, "
                         "  get()  { return _5[_2] } "
                         "})", 5, args, &exception);
        } else {
            V82JSC::exec(ctx,
                         "Object.defineProperty(_1, _2, "
                         "{ writable : !(_4&(1<<0)), "
                         "  enumerable : !(_4&(1<<1)), "
                         "  configurable : !(_4&(1<<2)), "
                         "  value: _3 })", 4, args, &exception);
        }
        if (exception.ShouldThow()) return MaybeLocal<Object>();
    }
    
    std::vector<PropAccessorImpl*> prop_accessors;
    for (auto i=m_property_accessors.Get(isolate); !i.IsEmpty(); ) {
        PropAccessorImpl *accessor = V82JSC::ToImpl<PropAccessorImpl>(i);
        prop_accessors.push_back(accessor);
        i = accessor->next_.Get(isolate);
    }
    for (auto i=prop_accessors.rbegin(); i!=prop_accessors.rend(); ++i) {
        PropAccessorImpl *accessor = (*i);
        MaybeLocal<Function> getter = !accessor->getter.IsEmpty() ? accessor->getter.Get(isolate)->GetFunction(context) : MaybeLocal<Function>();
        if (!accessor->getter.IsEmpty() && getter.IsEmpty()) return MaybeLocal<Object>();
        MaybeLocal<Function> setter = !accessor->setter.IsEmpty() ? accessor->setter.Get(isolate)->GetFunction(context) : MaybeLocal<Function>();
        if (!accessor->setter.IsEmpty() && setter.IsEmpty()) return MaybeLocal<Object>();
        thiz->SetAccessorProperty(accessor->name.Get(isolate),
                                  getter.ToLocalChecked(),
                                  setter.ToLocalChecked(),
                                  accessor->attribute,
                                  accessor->settings);
    }
    
    std::vector<ObjAccessorImpl*> accessors;
    for (auto i=m_accessors.Get(isolate); !i.IsEmpty(); ) {
        ObjAccessorImpl *accessor = V82JSC::ToImpl<ObjAccessorImpl>(i);
        accessors.push_back(accessor);
        i = accessor->next_.Get(isolate);
    }
    for (auto i=accessors.rbegin(); i!=accessors.rend(); ++i) {
        ObjAccessorImpl *accessor = (*i);
        Local<Value> data = accessor->data.Get(isolate);
        Local<ObjectImpl> thiz_ = * reinterpret_cast<Local<ObjectImpl>*>(&thiz);
        Maybe<bool> set = thiz_->SetAccessor(context,
                                             accessor->name.Get(isolate),
                                             accessor->getter,
                                             accessor->setter,
                                             data,
                                             accessor->settings,
                                             accessor->attribute,
                                             accessor->signature.Get(isolate));
        if (set.IsNothing()) return MaybeLocal<Object>();
    }
    
    std::vector<IntrinsicPropImpl*> intrinsics;
    for (auto i=m_intrinsics.Get(isolate); !i.IsEmpty(); ) {
        IntrinsicPropImpl *intrinsic = V82JSC::ToImpl<IntrinsicPropImpl>(i);
        intrinsics.push_back(intrinsic);
        i = intrinsic->next_.Get(isolate);
    }
    for (auto i=intrinsics.rbegin(); i!=intrinsics.rend(); ++i) {
        IntrinsicPropImpl *intrinsic = (*i);
        JSValueRef args[] = {
            instance,
            V82JSC::ToJSValueRef(intrinsic->name.Get(isolate), context)
        };
        switch(intrinsic->value) {
            case Intrinsic::kIteratorPrototype: {
                V82JSC::exec(ctx, "_1[_2] = [][Symbol.iterator]().__proto__.__proto__", 2, args, &exception);
                break;
            }
            case Intrinsic::kErrorPrototype: {
                V82JSC::exec(ctx, "_1[_2] = Error.prototype", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_keys: {
                V82JSC::exec(ctx, "_1[_2] = Array.prototype.keys", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_values: {
                V82JSC::exec(ctx, "_1[_2] = Array.prototype[Symbol.iterator]", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_entries: {
                V82JSC::exec(ctx, "_1[_2] = Array.prototype.entries", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_forEach: {
                V82JSC::exec(ctx, "_1[_2] = Array.prototype.forEach", 2, args, &exception);
                break;
            }
            default: {
                assert(0);
            }
        }
    }
    
    return thiz;
}
