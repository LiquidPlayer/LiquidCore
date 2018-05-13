//
//  ObjectTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include <string.h>

using namespace v8;

/** Creates an ObjectTemplate. */
Local<ObjectTemplate> ObjectTemplate::New(
                                 Isolate* isolate,
                                 Local<FunctionTemplate> constructor)
{
    auto destructor = [](InternalObjectImpl *o)
    {
        ObjectTemplateImpl *otempl = static_cast<ObjectTemplateImpl*>(o);
        v8::Isolate* i = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(o));
        JSContextRef ctx = V82JSC::ToContextRef(i);
        if (otempl->m_indexed_data) JSValueUnprotect(ctx, otempl->m_indexed_data);
        if (otempl->m_named_data) JSValueUnprotect(ctx, otempl->m_named_data);
        otempl->m_constructor_template.Reset();
        templateDestructor(o);
    };
    
    if (!constructor.IsEmpty()) {
        return constructor->InstanceTemplate();
    } else {
        ObjectTemplateImpl *otempl = (ObjectTemplateImpl*) TemplateImpl::New(isolate, sizeof(ObjectTemplateImpl), destructor);
        V82JSC::Map(otempl)->set_instance_type(v8::internal::OBJECT_TEMPLATE_INFO_TYPE);

        otempl->m_constructor_template = Copyable(v8::FunctionTemplate)();
        otempl->m_named_data = 0;
        otempl->m_indexed_data = 0;

        return V82JSC::CreateLocal<ObjectTemplate>(isolate, otempl);
    }
}

/** Get a template included in the snapshot by index. */
MaybeLocal<ObjectTemplate> ObjectTemplate::FromSnapshot(Isolate* isolate,
                                               size_t index)
{
    assert(0);
    return MaybeLocal<ObjectTemplate>();
}

/** Creates a new instance of this template.*/
MaybeLocal<Object> ObjectTemplate::NewInstance(Local<Context> context)
{
    ObjectTemplateImpl *impl = V82JSC::ToImpl<ObjectTemplateImpl>(this);
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    
    LocalException exception(iso);
    
    JSObjectRef instance = 0;
    if (!impl->m_constructor_template.IsEmpty()) {
        MaybeLocal<Function> ctor = impl->m_constructor_template.Get(isolate)->GetFunction(context);
        if (!ctor.IsEmpty()) {
            JSValueRef ctor_func = V82JSC::ToJSValueRef(ctor.ToLocalChecked(), context);
            instance = JSObjectCallAsConstructor(ctx->m_ctxRef, (JSObjectRef)ctor_func, 0, 0, &exception);
        }
    } else {
        instance = JSObjectMake(ctx->m_ctxRef, nullptr, nullptr);
    }
    if (!instance) {
        return MaybeLocal<Object>();
    }
    return impl->NewInstance(context, instance);
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)
#define CALLBACK_PARAMS JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, \
size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception
#define PASS ctx, function, thisObject, argumentCount, arguments, exception

template <typename V>
JSValueRef PropertyHandler(CALLBACK_PARAMS,
                           void (*named_handler)(const ObjectTemplateImpl*, Local<Name>, Local<Value>, PropertyCallbackInfo<V>&),
                           void (*indexed_handler)(const ObjectTemplateImpl*, uint32_t, Local<Value>, PropertyCallbackInfo<V>&))
{
    // Arguments:
    //  get            - target, property, receiver        -> Value
    //  set            - target, property, value, receiver -> True (assigned), False (not assigned)
    //  deleteProperty - target, property                  -> True (deleted), False (not deleted)
    //  has            - target, property                  -> True (has), False (not has)
    //  ownKeys        - target                            -> Array of keys
    *exception = 0;
    assert(argumentCount > 0);
    JSValueRef excp = 0;
    JSObjectRef target = (JSObjectRef) arguments[0];
    JSStringRef propertyName = 0;
    bool isSymbol = false;
    int index = 0;
    char *p = nullptr;
    if (argumentCount > 1) {
        isSymbol = JSValueToBoolean(ctx, V82JSC::exec(ctx, "return typeof _1 === 'symbol'", 1, &arguments[1]));
        if (!isSymbol) {
            propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
        }
        assert(excp==0);
    } else {
        propertyName = JSStringCreateWithUTF8CString("NONE");
    }
    if (!isSymbol) {
        size_t size = JSStringGetMaximumUTF8CStringSize(propertyName);
        char property[size];
        JSStringGetUTF8CString(propertyName, property, size);
        index = strtod(property, &p);
        if (p && (!strcmp(p, "constructor") || !strcmp(p, "__proto__") )) {
            return NULL;
        }
    }

    JSValueRef value;
    if (argumentCount > 2) {
        value = arguments[2];
    } else {
        value = JSValueMakeUndefined(ctx);
    }
    InstanceWrap* wrap = V82JSC::getPrivateInstance(ctx, target);
    Isolate *isolate = V82JSC::ToIsolate(wrap->m_isolate);
    HandleScope scope(isolate);
    
    const ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl>(wrap->m_object_template.Get(isolate));
    IsolateImpl *isolateimpl = wrap->m_isolate;
    Local<Context> context = ContextImpl::New(V82JSC::ToIsolate(isolateimpl), ctx);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);

    Local<Value> data;
    if (isSymbol || p!=nullptr) { /* Is named */
        data = ValueImpl::New(ctximpl, templ->m_named_data);
    } else { /* Is Indexed */
        data = ValueImpl::New(ctximpl, templ->m_indexed_data);
    }
    
    Local<Value> thiz = ValueImpl::New(ctximpl, target);
    
    v8::internal::Object * implicit[] = {
        0 /*FIXME*/,                                         // kShouldThrowOnErrorIndex = 0;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 1;
        O(isolateimpl),                                      // kIsolateIndex = 2;
        O(isolateimpl->i.roots.undefined_value),             // kReturnValueDefaultValueIndex = 3;
        O(isolateimpl->i.roots.the_hole_value),              // kReturnValueIndex = 4;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 5;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kThisIndex = 6;
    };
    
    PropertyCallbackImpl<V> info(implicit);
    Local<Value> set = ValueImpl::New(ctximpl, value);
    
    isolateimpl->i.ii.thread_local_top()->scheduled_exception_ = *isolateimpl->i.roots.the_hole_value;
    TryCatch try_catch(V82JSC::ToIsolate(isolateimpl));

    if (isSymbol || p!=nullptr) {
        named_handler(templ,
                      ValueImpl::New(ctximpl, arguments[1]).As<Name>(),
                      set, info);
    } else {
        indexed_handler(templ, index, set, info);
    }
    
    if (try_catch.HasCaught()) {
        *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
    } else if (isolateimpl->i.ii.thread_local_top()->scheduled_exception_ != *isolateimpl->i.roots.the_hole_value) {
        internal::Object * excep = isolateimpl->i.ii.thread_local_top()->scheduled_exception_;
        if (excep->IsHeapObject()) {
            ValueImpl* i = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(excep) - internal::kHeapObjectTag);
            *exception = i->m_value;
        } else {
            *exception = JSValueMakeNumber(ctx, internal::Smi::ToInt(excep));
        }
        isolateimpl->i.ii.thread_local_top()->scheduled_exception_ = reinterpret_cast<v8::internal::Object*>(isolateimpl->i.roots.the_hole_value);
    }

    if (implicit[4] == O(isolateimpl->i.roots.the_hole_value)) {
        return NULL;
    }
    
    Local<Value> retVal = info.GetReturnValue().Get();
    return V82JSC::ToJSValueRef<Value>(retVal, context);
}

InstanceWrap::~InstanceWrap()
{
    Isolate* isolate = reinterpret_cast<Isolate*>(m_isolate);
    HandleScope scope(isolate);
    
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    if (m_num_internal_fields && m_internal_fields) {
        for (int i=0; i<m_num_internal_fields; i++) {
            if (m_internal_fields[i]) JSValueUnprotect(ctx, m_internal_fields[i]);
        }
        delete m_internal_fields;
    }
    if (m_private_properties) {
        JSValueUnprotect(ctx, m_private_properties);
    }
    m_object_template.Reset();
}

v8::MaybeLocal<v8::Object> ObjectTemplateImpl::NewInstance(v8::Local<v8::Context> context, JSObjectRef root)
{
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    
    LocalException exception(iso);
    Local<ObjectTemplate> thiz = V82JSC::CreateLocal<ObjectTemplate>(isolate, this);
    
    // Structure:
    //
    // proxy -----> root . Symbol.for('org.liquidplayer.javascript.__v82jsc_private__') -->  lifecycle_object(wrap) --> InstanceWrap*
    
    // Create lifecycle object
    InstanceWrap *wrap = V82JSC::makePrivateInstance(ctx->m_ctxRef, root);
    wrap->m_isolate = iso;
    wrap->m_object_template = Copyable(ObjectTemplate)(isolate, thiz);
    wrap->m_num_internal_fields = m_internal_fields;
    wrap->m_internal_fields = new JSValueRef[m_internal_fields]();

    // Create proxy
    JSObjectRef handler = 0;
    if (m_need_proxy) {
        handler = JSObjectMake(ctx->m_ctxRef, nullptr, nullptr);
        auto handler_func = [ctx, handler](const char *name, JSObjectCallAsFunctionCallback callback) -> void {
            JSValueRef excp = 0;
            JSStringRef sname = JSStringCreateWithUTF8CString(name);
            JSObjectRef f = JSObjectMakeFunctionWithCallback(ctx->m_ctxRef, sname, callback);
            JSObjectSetProperty(ctx->m_ctxRef, handler, sname, f, 0, &excp);
            JSStringRelease(sname);
            assert(excp==0);
        };
        
        handler_func("apply", [](CALLBACK_PARAMS) -> JSValueRef
        {
            return NULL;
        });
        handler_func("get", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Value>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_named_handler.getter)
                    impl->m_named_handler.getter(property, info);
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_indexed_handler.getter)
                    impl->m_indexed_handler.getter(index, info);
            });
            if (ret == NULL && !*exception) {
                // Not handled.  Pass thru.
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "return _1[_2]", 2, arguments, exception);
            }
            return ret;
        });
        handler_func("set", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Value>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_named_handler.setter) {
                    impl->m_named_handler.setter(property, value, info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Value>& info)
            {
                if (impl->m_indexed_handler.setter) {
                    impl->m_indexed_handler.setter(index, value, info);
                }
            });
            if (*exception) {
                return JSValueMakeBoolean(ctx, false);
            }
            if (ret == NULL) {
                assert(argumentCount>2);
                return V82JSC::exec(ctx, "return _1[_2] = _3", 3, arguments, exception);
            }
            return JSValueMakeBoolean(ctx, true);
        });
        handler_func("has", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Integer>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_named_handler.query) {
                    impl->m_named_handler.query(property, info);
                } else if (impl->m_named_handler.getter) {
                    info.GetReturnValue().Set(v8::PropertyAttribute::None);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_indexed_handler.query) {
                    impl->m_indexed_handler.query(index, info);
                } else if (impl->m_indexed_handler.getter) {
                    info.GetReturnValue().Set(v8::PropertyAttribute::None);
                }
            });
            if (*exception) {
                return JSValueMakeBoolean(ctx, false);
            }
            if (ret == NULL) {
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "return _1.hasOwnProperty(_2)", 2, arguments, exception);
            }
            return JSValueMakeBoolean(ctx, !JSValueIsUndefined(ctx, ret));
        });
        handler_func("deleteProperty", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<v8::Boolean>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<v8::Boolean>& info)
            {
                if (impl->m_named_handler.deleter) {
                    impl->m_named_handler.deleter(property, info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<v8::Boolean>& info)
            {
                if (impl->m_indexed_handler.deleter) {
                    impl->m_indexed_handler.deleter(index, info);
                }
            });
            if (!*exception && ret == NULL) {
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "delete _1[_2]", 2, arguments, exception);
            }
            return ret;
        });
        handler_func("ownKeys", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<v8::Array>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<v8::Array>& info)
            {
                if (impl->m_named_handler.enumerator) {
                    impl->m_named_handler.enumerator(info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<v8::Array>& info)
            {
                if (impl->m_indexed_handler.enumerator) {
                    impl->m_indexed_handler.enumerator(info);
                }
            });
            if (!*exception && ret == NULL) {
                assert(argumentCount>0);
                return V82JSC::exec(ctx, "return Object.getOwnPropertyNames(_1)", 1, arguments, exception);
            }
            return ret;
        });
        handler_func("getPrototypeOf", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>0);
            return JSObjectGetPrototype(ctx, (JSObjectRef) arguments[0]);
        });
        handler_func("setPrototypeOf", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>1);
            JSObjectSetPrototype(ctx, (JSObjectRef) arguments[0], arguments[1]);
            return JSValueMakeBoolean(ctx, true);
        });
        handler_func("getOwnPropertyDescriptor", [](CALLBACK_PARAMS) -> JSValueRef {
            assert(argumentCount>1);
            return V82JSC::exec(ctx, "return Object.getOwnPropertyDescriptor(_1, _2)", 2, arguments, exception);
        });
    }

    MaybeLocal<Object> instance;
    if (!m_constructor_template.IsEmpty()) {
        instance = InitInstance(context, root, exception, m_constructor_template.Get(isolate));
    } else {
        instance = InitInstance(context, root, exception);
    }
    if (instance.IsEmpty()) {
        return instance;
    }
    if (m_need_proxy) {
        JSValueRef args[] = {root, handler};
        return ValueImpl::New(ctx, V82JSC::exec(ctx->m_ctxRef, "return new Proxy(_1, _2)", 2, args)).As<Object>();
    }
    
    return instance;
}

/**
 * Sets an accessor on the object template.
 *
 * Whenever the property with the given name is accessed on objects
 * created from this ObjectTemplate the getter and setter callbacks
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
void ObjectTemplate::SetAccessor(
                 Local<String> name, AccessorGetterCallback getter,
                 AccessorSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    SetAccessor(name.As<Name>(),
                reinterpret_cast<AccessorNameGetterCallback>(getter),
                reinterpret_cast<AccessorNameSetterCallback>(setter),
                data, settings, attribute, signature);
}
void ObjectTemplate::SetAccessor(
                 Local<Name> name, AccessorNameGetterCallback getter,
                 AccessorNameSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    ObjectTemplateImpl *this_ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(this_));
    
    ObjAccessor accessor;
    accessor.name = Copyable(Name)(isolate, name);
    accessor.getter = getter;
    accessor.setter = setter;
    accessor.data = Copyable(Value)(isolate, data);
    accessor.settings = settings;
    accessor.attribute = attribute;
    
    // For now, Signature and AccessorSignature are the same
    Local<Signature> sig = * reinterpret_cast<Local<Signature>*>(&signature);
    
    accessor.signature = Copyable(Signature)(isolate, sig);
    
    this_->m_accessors.push_back(accessor);
}

/**
 * Sets a named property handler on the object template.
 *
 * Whenever a property whose name is a string is accessed on objects created
 * from this object template, the provided callback is invoked instead of
 * accessing the property directly on the JavaScript object.
 *
 * SetNamedPropertyHandler() is different from SetHandler(), in
 * that the latter can intercept symbol-named properties as well as
 * string-named properties when called with a
 * NamedPropertyHandlerConfiguration. New code should use SetHandler().
 *
 * \param getter The callback to invoke when getting a property.
 * \param setter The callback to invoke when setting a property.
 * \param query The callback to invoke to check if a property is present,
 *   and if present, get its attributes.
 * \param deleter The callback to invoke when deleting a property.
 * \param enumerator The callback to invoke to enumerate all the named
 *   properties of an object.
 * \param data A piece of data that will be passed to the callbacks
 *   whenever they are invoked.
 */
// TODO(dcarney): deprecate
void ObjectTemplate::SetNamedPropertyHandler(NamedPropertyGetterCallback getter,
                             NamedPropertySetterCallback setter,
                             NamedPropertyQueryCallback query,
                             NamedPropertyDeleterCallback deleter,
                             NamedPropertyEnumeratorCallback enumerator,
                             Local<Value> data)
{
    assert(0);
}

/**
 * Sets a named property handler on the object template.
 *
 * Whenever a property whose name is a string or a symbol is accessed on
 * objects created from this object template, the provided callback is
 * invoked instead of accessing the property directly on the JavaScript
 * object.
 *
 * @param configuration The NamedPropertyHandlerConfiguration that defines the
 * callbacks to invoke when accessing a property.
 */
void ObjectTemplate::SetHandler(const NamedPropertyHandlerConfiguration& configuration)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    Local<Value> data = configuration.data;
    templ->m_named_handler = configuration;
    templ->m_named_handler.data.Clear();

    if (data.IsEmpty()) {
        data = Undefined(Isolate::GetCurrent());
    }
    templ->m_named_data = V82JSC::ToJSValueRef(configuration.data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToContextRef(Isolate::GetCurrent()), templ->m_named_data);
    templ->m_need_proxy = true;
}

/**
 * Sets an indexed property handler on the object template.
 *
 * Whenever an indexed property is accessed on objects created from
 * this object template, the provided callback is invoked instead of
 * accessing the property directly on the JavaScript object.
 *
 * @param configuration The IndexedPropertyHandlerConfiguration that defines
 * the callbacks to invoke when accessing a property.
 */
void ObjectTemplate::SetHandler(const IndexedPropertyHandlerConfiguration& configuration)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    Local<Value> data = configuration.data;
    templ->m_indexed_handler = configuration;
    templ->m_indexed_handler.data.Clear();

    if (data.IsEmpty()) {
        data = Undefined(Isolate::GetCurrent());
    }
    templ->m_indexed_data = V82JSC::ToJSValueRef(configuration.data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToContextRef(Isolate::GetCurrent()), templ->m_indexed_data);
    templ->m_need_proxy = true;
}

/**
 * Sets the callback to be used when calling instances created from
 * this template as a function.  If no callback is set, instances
 * behave like normal JavaScript objects that cannot be called as a
 * function.
 */
void ObjectTemplate::SetCallAsFunctionHandler(FunctionCallback callback,
                              Local<Value> data)
{
    assert(0);
}

/**
 * Mark object instances of the template as undetectable.
 *
 * In many ways, undetectable objects behave as though they are not
 * there.  They behave like 'undefined' in conditionals and when
 * printed.  However, properties can be accessed and called as on
 * normal objects.
 */
void ObjectTemplate::MarkAsUndetectable()
{
    assert(0);
}

/**
 * Sets access check callback on the object template and enables access
 * checks.
 *
 * When accessing properties on instances of this object template,
 * the access check callback will be called to determine whether or
 * not to allow cross-context access to the properties.
 */
void ObjectTemplate::SetAccessCheckCallback(AccessCheckCallback callback,
                            Local<Value> data)
{
    assert(0);
}

/**
 * Like SetAccessCheckCallback but invokes an interceptor on failed access
 * checks instead of looking up all-can-read properties. You can only use
 * either this method or SetAccessCheckCallback, but not both at the same
 * time.
 */
void ObjectTemplate::SetAccessCheckCallbackAndHandler(
                                      AccessCheckCallback callback,
                                      const NamedPropertyHandlerConfiguration& named_handler,
                                      const IndexedPropertyHandlerConfiguration& indexed_handler,
                                      Local<Value> data)
{
    assert(0);
}

/**
 * Gets the number of internal fields for objects generated from
 * this template.
 */
int ObjectTemplate::InternalFieldCount()
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    return templ->m_internal_fields;
}

/**
 * Sets the number of internal fields for objects generated from
 * this template.
 */
void ObjectTemplate::SetInternalFieldCount(int value)
{
    ObjectTemplateImpl *templ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    templ->m_internal_fields = value > 0 ? value : 0;
}

/**
 * Returns true if the object will be an immutable prototype exotic object.
 */
bool ObjectTemplate::IsImmutableProto()
{
    assert(0);
    return false;
}

/**
 * Makes the ObjectTempate for an immutable prototype exotic object, with an
 * immutable __proto__.
 */
void ObjectTemplate::SetImmutableProto()
{
    assert(0);
}

