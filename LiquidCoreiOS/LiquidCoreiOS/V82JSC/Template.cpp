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
    Isolate *isolate = V82JSC::ToIsolate(this);
    
    Prop prop;
    prop.name.Reset(isolate, name);
    prop.value.Reset(isolate, value);
    prop.attributes = attributes;
    
    this_->m_properties.push_back(std::move(prop));
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

    PropAccessor accessor;
    accessor.name.Reset(isolate, name);
    accessor.getter.Reset(isolate, getter);
    accessor.setter.Reset(isolate, setter);
    accessor.attribute = attribute;
    accessor.settings = settings;
    
    this_->m_property_accessors.push_back(std::move(accessor));
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
    IntrinsicProp prop;
    prop.name.Reset(isolate, name);
    prop.value = intrinsic;
    templ->m_intrinsics.push_back(std::move(prop));
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
    TemplateWrap *wrap = reinterpret_cast<TemplateWrap*>(JSObjectGetPrivate(proxy_function));
    IsolateImpl* isolateimpl = wrap->m_isolate;
    Isolate* isolate = V82JSC::ToIsolate(isolateimpl);
    HandleScope scope(isolate);
    
    Local<Context> context = ContextImpl::New(V82JSC::ToIsolate(isolateimpl), ctx);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    TemplateImpl *templ = V82JSC::ToImpl<TemplateImpl>(wrap->m_template.Get(isolate));
    
    if (V82JSC::Map(templ)->instance_type() == internal::OBJECT_TEMPLATE_INFO_TYPE) {
        thisObject = proxy_function;
    }
    Local<Value> thiz = ValueImpl::New(ctximpl, thisObject);
    Local<Value> holder = thiz;

    // Check signature
    bool signature_match = templ->m_signature.IsEmpty();
    if (!signature_match) {
        SignatureImpl *sig = V82JSC::ToImpl<SignatureImpl>(templ->m_signature.Get(isolate));
        const TemplateImpl *sig_templ = V82JSC::ToImpl<FunctionTemplateImpl>(sig->m_template.Get(isolate));
        JSValueRef proto = thisObject;
        InstanceWrap* thisWrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)proto);
        while(!signature_match && JSValueIsObject(ctx, proto)) {
            if (thisWrap && (proto == thisObject || thisWrap->m_isHiddenPrototype)) {
                holder = proto == thisObject? thiz : ValueImpl::New(ctximpl, proto);
                ObjectTemplateImpl* ot = V82JSC::ToImpl<ObjectTemplateImpl>(thisWrap->m_object_template.Get(isolate));
                Local<FunctionTemplate> ctort = ot->m_constructor_template.Get(isolate);
                const TemplateImpl *templ = ctort.IsEmpty() ? nullptr : V82JSC::ToImpl<FunctionTemplateImpl>(ctort);
                while (!signature_match && templ) {
                    signature_match = sig_templ == templ;
                    templ = templ->m_parent.IsEmpty() ? nullptr : V82JSC::ToImpl<FunctionTemplateImpl>(templ->m_parent.Get(isolate));
                }
            }
            proto = V82JSC::GetRealPrototype(context, (JSObjectRef)proto);
            thisWrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)proto);
            if (!thisWrap || !thisWrap->m_isHiddenPrototype) break;
        }
    }
    if (!signature_match) {
        JSStringRef message = JSStringCreateWithUTF8CString("new TypeError('Illegal invocation')");
        *exception = JSEvaluateScript(ctx, message, 0, 0, 0, 0);
        JSStringRelease(message);
        return 0;
    }
    Local<Value> data = ValueImpl::New(ctximpl, templ->m_data);
    typedef v8::internal::Heap::RootListIndex R;
    internal::Object *the_hole = isolateimpl->ii.heap()->root(R::kTheHoleValueRootIndex);
    internal::Object *target = isolateimpl->ii.heap()->root(R::kUndefinedValueRootIndex);
    
    if (isConstructCall) {
        JSValueRef t = JSObjectMake(ctx, 0, 0);
        target = * reinterpret_cast<v8::internal::Object**> (*ValueImpl::New(ctximpl, t));
    }

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*holder), // kHolderIndex = 0;
        O(isolateimpl),                                      // kIsolateIndex = 1;
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
    
    isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    
    FunctionCallbackImpl info(implicit, values, (int) argumentCount);
    TryCatch try_catch(V82JSC::ToIsolate(isolateimpl));
    
    if (templ->m_callback) {
        templ->m_callback(info);
    }

    if (try_catch.HasCaught()) {
        *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
    } else if (isolateimpl->ii.thread_local_top()->scheduled_exception_ != the_hole) {
        internal::Object * excep = isolateimpl->ii.thread_local_top()->scheduled_exception_;
        if (excep->IsHeapObject()) {
            ValueImpl* i = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(excep) - internal::kHeapObjectTag);
            *exception = i->m_value;
        } else {
            *exception = JSValueMakeNumber(ctx, internal::Smi::ToInt(excep));
        }
        isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    }
    
    Local<Value> ret = info.GetReturnValue().Get();
    
    return (T) V82JSC::ToJSValueRef<Value>(ret, context);
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


void templateDestructor(InternalObjectImpl *o)
{
    TemplateImpl * templ = static_cast<TemplateImpl*>(o);
    
    templ->m_prototype_template.Reset();
    templ->m_signature.Reset();
    if (templ->m_data) {
        v8::Isolate* i = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(o));
        JSContextRef ctx = V82JSC::ToContextRef(i);
        JSValueUnprotect(ctx, templ->m_data);
    }
    templ->m_parent.Reset();
    templ->m_accessors.clear();
    templ->m_property_accessors.clear();
    templ->m_properties.clear();
}

/** Creates a function template.*/
TemplateImpl* TemplateImpl::New(Isolate* isolate, size_t size, InternalObjectDestructor destructor)
{
    TemplateImpl * templ = static_cast<TemplateImpl *>(HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate), size, destructor));
    
    templ->m_properties = std::vector<Prop>();
    templ->m_property_accessors = std::vector<PropAccessor>();
    templ->m_accessors = std::vector<ObjAccessor>();
    templ->m_intrinsics = std::vector<IntrinsicProp>();
    templ->m_parent = Copyable(FunctionTemplate)();
    templ->m_callback = nullptr;
    templ->m_data = 0;
    templ->m_signature = Copyable(v8::Signature)();
    templ->m_prototype_template = Copyable(v8::ObjectTemplate)();

    return templ;
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
        thiz = instance_impl->InitInstance(context, instance, excep);
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
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, instance);
    
    Local<Object> real_properties = Object::New(isolate);

    for (auto i=m_properties.begin(); i!=m_properties.end(); ++i) {
        typedef internal::Object O;
        typedef internal::Internals I;
        Local<Data> vv = i->value.Get(isolate);
        O* obj = * reinterpret_cast<O* const *>(*vv);
        JSValueRef v = V82JSC::ToJSValueRef(vv, context);
        if (obj->IsHeapObject()) {
            int t = I::GetInstanceType(obj);
            if (t == v8::internal::FUNCTION_TEMPLATE_INFO_TYPE) {
                FunctionTemplateImpl *fti = V82JSC::ToImpl<FunctionTemplateImpl>(vv);
                Local<FunctionTemplate> ft = V82JSC::CreateLocal<FunctionTemplate>(isolate, fti);
                MaybeLocal<Function> o = ft->GetFunction(context);
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
            V82JSC::ToJSValueRef(i->name.Get(isolate), context),
            v,
            JSValueMakeNumber(ctx, i->attributes),
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
    
    for (auto i=m_property_accessors.begin(); i!=m_property_accessors.end(); ++i) {
        MaybeLocal<Function> getter = !i->getter.IsEmpty() ? i->getter.Get(isolate)->GetFunction(context) : MaybeLocal<Function>();
        if (!i->getter.IsEmpty() && getter.IsEmpty()) return MaybeLocal<Object>();
        MaybeLocal<Function> setter = !i->setter.IsEmpty() ? i->setter.Get(isolate)->GetFunction(context) : MaybeLocal<Function>();
        if (!i->setter.IsEmpty() && setter.IsEmpty()) return MaybeLocal<Object>();
        thiz->SetAccessorProperty(i->name.Get(isolate),
                                  getter.ToLocalChecked(),
                                  setter.ToLocalChecked(),
                                  i->attribute,
                                  i->settings);
    }
    
    for (auto i=m_accessors.begin(); i!=m_accessors.end(); ++i) {
        Local<Value> data = i->data.Get(isolate);
        Maybe<bool> set = thiz->SetAccessor(context,
                                            i->name.Get(isolate),
                                            i->getter,
                                            i->setter,
                                            data,
                                            i->settings,
                                            i->attribute);
        if (set.IsNothing()) return MaybeLocal<Object>();
    }
    
    for (auto i=m_intrinsics.begin(); i!=m_intrinsics.end(); ++i) {
        JSValueRef args[] = {
            instance,
            V82JSC::ToJSValueRef(i->name.Get(isolate), context)
        };
        switch(i->value) {
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
