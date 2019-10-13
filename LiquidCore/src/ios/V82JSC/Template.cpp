/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSCPrivate.h"
#include "ObjectTemplate.h"
#include "FunctionTemplate.h"
#include "Object.h"

using namespace V82JSC;
using namespace v8;

/**
 * Adds a property to each instance created by this template.
 *
 * The property must be defined either as a primitive value, or a template.
 */
void v8::Template::Set(Local<Name> name, Local<Data> value, PropertyAttribute attributes)
{
    auto this_ = ToImpl<V82JSC::Template,Template>(this);
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);

    auto prop = static_cast<V82JSC::Prop *>
    (HeapAllocator::Alloc(ToIsolateImpl(this_), ToIsolateImpl(this_)->m_property_accessor_map));

    prop->name.Reset(isolate, name);
    prop->value.Reset(isolate, value);
    prop->attributes = attributes;
    prop->next_.Reset(isolate, this_->m_properties.Get(isolate));
    Local<v8::Prop> local = CreateLocal<v8::Prop>(isolate, prop);
    this_->m_properties.Reset(isolate, local);
}
void v8::Template::SetPrivate(Local<Private> name, Local<Data> value, PropertyAttribute attributes)
{
    assert(0);
}

void v8::Template::SetAccessorProperty(
                         Local<Name> name,
                         Local<FunctionTemplate> getter,
                         Local<FunctionTemplate> setter,
                         PropertyAttribute attribute,
                         AccessControl settings)
{
    auto this_ = ToImpl<V82JSC::Template,Template>(this);
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);

    auto accessor = static_cast<V82JSC::PropAccessor *>
    (HeapAllocator::Alloc(ToIsolateImpl(this_), ToIsolateImpl(this_)->m_property_accessor_map));

    accessor->name.Reset(isolate, name);
    accessor->getter.Reset(isolate, getter);
    accessor->setter.Reset(isolate, setter);
    accessor->attribute = attribute;
    accessor->settings = settings;
    accessor->next_.Reset(isolate, this_->m_property_accessors.Get(isolate));
    Local<v8::PropAccessor> local = CreateLocal<v8::PropAccessor>(isolate, accessor);
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
void v8::Template::SetNativeDataProperty(
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

void v8::Template::SetNativeDataProperty(
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
void v8::Template::SetLazyDataProperty(Local<Name> name, AccessorNameGetterCallback getter,
                         Local<Value> data,
                         PropertyAttribute attribute)
{
    assert(0);
}

/**
 * During template instantiation, sets the value with the intrinsic property
 * from the correct context.
 */
void v8::Template::SetIntrinsicDataProperty(Local<Name> name, Intrinsic intrinsic,
                              PropertyAttribute attribute)
{
    auto templ = ToImpl<V82JSC::Template, Template>(this);
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    
    auto prop = static_cast<V82JSC::IntrinsicProp *>
    (HeapAllocator::Alloc(ToIsolateImpl(isolate), ToIsolateImpl(isolate)->m_intrinsic_property_map));
    
    prop->next_.Reset(isolate, templ->m_intrinsics.Get(isolate));
    Local<v8::IntrinsicProp> local = CreateLocal<v8::IntrinsicProp>(isolate, prop);
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
    IsolateImpl *iso = IsolateFromCtx(ctx);
    Isolate *isolate = ToIsolate(iso);
    v8::Locker lock(isolate);

    HandleScope scope(isolate);
    
    Local<v8::Template> local = FromPersistentData<v8::Template>(isolate, JSObjectGetPrivate(proxy_function));
    auto thread = IsolateImpl::PerThreadData::Get(iso);

    Local<v8::Context> context = LocalContext::New(isolate, ctx);
    v8::Context::Scope context_scope(context);
    auto ctximpl = ToContextImpl(context);
    auto templ = ToImpl<V82JSC::Template>(local);
    
    if (templ->m_map->instance_type() == internal::OBJECT_TEMPLATE_INFO_TYPE) {
        thisObject = proxy_function;
    }
    Local<v8::Value> thiz = V82JSC::Value::New(ctximpl, thisObject);
    Local<v8::Value> holder = thiz;

    // Check signature
    bool signature_match = templ->m_signature.IsEmpty();
    if (!signature_match) {
        auto sig = ToImpl<V82JSC::Signature>(templ->m_signature.Get(isolate));
        auto sig_templ = ToImpl<V82JSC::Template>(sig->m_template.Get(isolate));
        JSValueRef proto = thisObject;
        auto thisWrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)proto);
        while(!signature_match && JSValueIsObject(ctx, proto)) {
            if (thisWrap && !thisWrap->m_object_template.IsEmpty() && (proto == thisObject || thisWrap->m_isHiddenPrototype)) {
                holder = proto == thisObject? thiz : V82JSC::Value::New(ctximpl, proto);
                auto ot = ToImpl<V82JSC::ObjectTemplate>(thisWrap->m_object_template.Get(isolate));
                Local<v8::FunctionTemplate> ctort = ot->m_constructor_template.Get(isolate);
                auto templ = ctort.IsEmpty() ? nullptr : ToImpl<V82JSC::Template>(ctort);
                while (!signature_match && templ) {
                    signature_match = sig_templ == templ;
                    templ = templ->m_parent.IsEmpty() ? nullptr : ToImpl<V82JSC::Template>(templ->m_parent.Get(isolate));
                }
            }
            proto = GetRealPrototype(context, (JSObjectRef)proto);
            thisWrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)proto);
            if (!thisWrap || !thisWrap->m_isHiddenPrototype) break;
        }
    }
    if (!signature_match) {
        JSStringRef message = JSStringCreateWithUTF8CString("new TypeError('Illegal invocation')");
        *exception = JSEvaluateScript(ctx, message, 0, 0, 0, 0);
        JSStringRelease(message);
        return 0;
    }
    
    ++ thread->m_callback_depth;

    Local<v8::Value> data = V82JSC::Value::New(ctximpl, templ->m_data);
    typedef v8::internal::Heap::RootListIndex R;
    internal::Object *the_hole = iso->ii.heap()->root(R::kTheHoleValueRootIndex);
    internal::Object *target = iso->ii.heap()->root(R::kUndefinedValueRootIndex);
    
    if (isConstructCall) {
        JSValueRef t = JSObjectMake(ctx, 0, 0);
        target = * reinterpret_cast<v8::internal::Object**> (*V82JSC::Value::New(ctximpl, t));
    }

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*holder), // kHolderIndex = 0;
        O(iso),                                              // kIsolateIndex = 1;
        the_hole,                                            // kReturnValueDefaultValueIndex = 2;
        the_hole,                                            // kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        target                                               // kNewTargetIndex = 5;
    };

    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<v8::Value> arg = V82JSC::Value::New(ctximpl, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    thread->m_scheduled_exception = the_hole;
    
    V82JSC::FunctionCallback info(implicit, values, (int) argumentCount);
    TryCatch try_catch(ToIsolate(iso));
    
    if (templ->m_callback) {
        templ->m_callback(info);
    } else {
        info.GetReturnValue().Set(info.This());
    }

    if (try_catch.HasCaught()) {
        *exception = ToJSValueRef(try_catch.Exception(), context);
        if (!*exception && thread->m_scheduled_exception != the_hole) {
            internal::Object * excep = thread->m_scheduled_exception;
            *exception = ToJSValueRef_<v8::Value>(excep, context);
        }
        thread->m_scheduled_exception = the_hole;
    }

    -- thread->m_callback_depth;
    
    if (!*exception) {
        Local<v8::Value> ret = info.GetReturnValue().Get();

        return (T) ToJSValueRef<v8::Value>(ret, context);
    } else {
        return NULL;
    }
}

JSValueRef V82JSC::Template::callAsFunctionCallback(JSContextRef ctx,
                                                JSObjectRef proxy_function,
                                                JSObjectRef thisObject,
                                                size_t argumentCount,
                                                const JSValueRef *arguments,
                                                JSValueRef *exception)
{
    return callAsCallback<JSValueRef>(ctx, proxy_function, thisObject, argumentCount, arguments, exception);
}

JSObjectRef V82JSC::Template::callAsConstructorCallback(JSContextRef ctx,
                                                    JSObjectRef constructor,
                                                    size_t argumentCount,
                                                    const JSValueRef *arguments,
                                                    JSValueRef *exception)
{
    return callAsCallback<JSObjectRef>(ctx, constructor, 0, argumentCount, arguments, exception);
}

MaybeLocal<Object> V82JSC::Template::InitInstance(Local<v8::Context> context, JSObjectRef instance,
                                                  LocalException& excep, Local<v8::FunctionTemplate> ft)
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(ft);
    Isolate *isolate = ToIsolate(impl);
    EscapableHandleScope scope(isolate);
    
    Local<v8::ObjectTemplate> instance_template = ft->InstanceTemplate();
    auto instance_impl = ToImpl<V82JSC::ObjectTemplate>(instance_template);
    
    MaybeLocal<Object> thiz;
    if (!impl->m_parent.IsEmpty()) {
        thiz = InitInstance(context, instance, excep, impl->m_parent.Get(isolate));
    }
    if (!excep.ShouldThrow()) {
        thiz = reinterpret_cast<Template*>(instance_impl)->InitInstance(context, instance, excep);
        return scope.Escape(thiz.ToLocalChecked());
    }
    return MaybeLocal<Object>();
};

MaybeLocal<Object> V82JSC::Template::InitInstance(Local<v8::Context> context, JSObjectRef instance,
                                                  LocalException& exception)
{
    auto ctximpl = ToContextImpl(context);
    auto iso = ToIsolateImpl(ctximpl);
    auto isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    v8::Context::Scope context_scope(context);
    
    JSContextRef ctx = ToContextRef(context);
    Local<Object> thiz = V82JSC::Value::New(ctximpl, instance).As<Object>();
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, instance);
    
    Local<Object> real_properties = Object::New(isolate);

    // Reverse lists so last added properties take precendence
    std::vector<Prop*> props;
    for (auto i=m_properties.Get(isolate); !i.IsEmpty(); ) {
        auto prop = ToImpl<V82JSC::Prop>(i);
        props.push_back(prop);
        i = prop->next_.Get(isolate);
    }
    for (auto i=props.rbegin(); i!=props.rend(); ++i) {
        auto prop = (*i);
        typedef internal::Object O;
        typedef internal::Internals I;
        Local<Data> vv = prop->value.Get(isolate);
        O* obj = * reinterpret_cast<O* const *>(*vv);
        JSValueRef v = ToJSValueRef(vv, context);
        if (obj->IsHeapObject()) {
            int t = I::GetInstanceType(obj);
            if (t == v8::internal::FUNCTION_TEMPLATE_INFO_TYPE) {
                auto fti = ToImpl<V82JSC::FunctionTemplate>(vv);
                Local<v8::FunctionTemplate> ft = CreateLocal<v8::FunctionTemplate>(isolate, fti);
                MaybeLocal<Function> o = FunctionTemplate::GetFunction(*ft, context, prop->name.Get(isolate));
                if (!o.IsEmpty()) {
                    v = ToJSValueRef<Function>(o.ToLocalChecked(), context);
                }
            } else if (t == v8::internal::OBJECT_TEMPLATE_INFO_TYPE) {
                auto oti = ToImpl<V82JSC::ObjectTemplate>(vv);
                Local<v8::ObjectTemplate> ot = CreateLocal<v8::ObjectTemplate>(isolate, oti);
                MaybeLocal<Object> o = ot->NewInstance(context);
                if (!o.IsEmpty()) {
                    v = ToJSValueRef<Object>(o.ToLocalChecked(), context);
                }
            }
        }
        JSValueRef args[] = {
            instance,
            ToJSValueRef(prop->name.Get(isolate), context),
            v,
            JSValueMakeNumber(ctx, prop->attributes),
            ToJSValueRef(real_properties, context)
        };
        
        /* None = 0,
           ReadOnly = 1 << 0,
           DontEnum = 1 << 1,
           DontDelete = 1 << 2
        */
        
        if (wrap->m_isHiddenPrototype) {
            exec(ctx,
                         "_5[_2] = _3; "
                         "Object.defineProperty(_1, _2, "
                         "{ enumerable : !(_4&(1<<1)), "
                         "  configurable : !(_4&(1<<2)), "
                         "  set(v) { return _5[_2] = v; }, "
                         "  get()  { return _5[_2] } "
                         "})", 5, args, &exception);
        } else {
            exec(ctx,
                         "Object.defineProperty(_1, _2, "
                         "{ writable : !(_4&(1<<0)), "
                         "  enumerable : !(_4&(1<<1)), "
                         "  configurable : !(_4&(1<<2)), "
                         "  value: _3 })", 4, args, &exception);
        }
        if (exception.ShouldThrow()) return MaybeLocal<Object>();
    }
    
    std::vector<PropAccessor*> prop_accessors;
    for (auto i=m_property_accessors.Get(isolate); !i.IsEmpty(); ) {
        auto accessor = ToImpl<V82JSC::PropAccessor>(i);
        prop_accessors.push_back(accessor);
        i = accessor->next_.Get(isolate);
    }
    for (auto i=prop_accessors.rbegin(); i!=prop_accessors.rend(); ++i) {
        auto accessor = (*i);
        MaybeLocal<Function> getter = !accessor->getter.IsEmpty() ? accessor->getter.Get(isolate)->GetFunction(context) : MaybeLocal<Function>();
        if (!accessor->getter.IsEmpty() && getter.IsEmpty()) return MaybeLocal<Object>();
        MaybeLocal<Function> setter = !accessor->setter.IsEmpty() ? accessor->setter.Get(isolate)->GetFunction(context) : MaybeLocal<Function>();
        if (!accessor->setter.IsEmpty() && setter.IsEmpty()) return MaybeLocal<Object>();
        auto checkedGetter = getter.IsEmpty() ? Local<Function>() : getter.ToLocalChecked();
        auto checkedSetter = setter.IsEmpty() ? Local<Function>() : setter.ToLocalChecked();
        thiz->SetAccessorProperty(accessor->name.Get(isolate),
                                  checkedGetter,
                                  checkedSetter,
                                  accessor->attribute,
                                  accessor->settings);
    }
    
    std::vector<ObjAccessor*> accessors;
    for (auto i=m_accessors.Get(isolate); !i.IsEmpty(); ) {
        auto accessor = ToImpl<V82JSC::ObjAccessor>(i);
        accessors.push_back(accessor);
        i = accessor->next_.Get(isolate);
    }
    for (auto i=accessors.rbegin(); i!=accessors.rend(); ++i) {
        auto accessor = (*i);
        Local<v8::Value> data = accessor->data.Get(isolate);
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
    
    std::vector<IntrinsicProp*> intrinsics;
    for (auto i=m_intrinsics.Get(isolate); !i.IsEmpty(); ) {
        auto intrinsic = ToImpl<V82JSC::IntrinsicProp>(i);
        intrinsics.push_back(intrinsic);
        i = intrinsic->next_.Get(isolate);
    }
    for (auto i=intrinsics.rbegin(); i!=intrinsics.rend(); ++i) {
        auto intrinsic = (*i);
        JSValueRef args[] = {
            instance,
            ToJSValueRef(intrinsic->name.Get(isolate), context)
        };
        switch(intrinsic->value) {
            case Intrinsic::kIteratorPrototype: {
                exec(ctx, "_1[_2] = [][Symbol.iterator]().__proto__.__proto__", 2, args, &exception);
                break;
            }
            case Intrinsic::kErrorPrototype: {
                exec(ctx, "_1[_2] = Error.prototype", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_keys: {
                exec(ctx, "_1[_2] = Array.prototype.keys", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_values: {
                exec(ctx, "_1[_2] = Array.prototype[Symbol.iterator]", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_entries: {
                exec(ctx, "_1[_2] = Array.prototype.entries", 2, args, &exception);
                break;
            }
            case Intrinsic::kArrayProto_forEach: {
                exec(ctx, "_1[_2] = Array.prototype.forEach", 2, args, &exception);
                break;
            }
            default: {
                assert(0);
            }
        }
    }
    
    return scope.Escape(thiz);
}
