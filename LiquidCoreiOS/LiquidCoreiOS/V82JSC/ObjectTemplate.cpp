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
    if (!constructor.IsEmpty()) {
        return constructor->InstanceTemplate();
    } else {
        ObjectTemplateImpl *otempl = (ObjectTemplateImpl*) TemplateImpl::New(isolate, sizeof(ObjectTemplateImpl));
        otempl->pMap->set_instance_type(v8::internal::OBJECT_TEMPLATE_INFO_TYPE);

        return _local<ObjectTemplate>(otempl).toLocal();
    }
}

/** Get a template included in the snapshot by index. */
MaybeLocal<ObjectTemplate> ObjectTemplate::FromSnapshot(Isolate* isolate,
                                               size_t index)
{
    return MaybeLocal<ObjectTemplate>();
}

/** Creates a new instance of this template.*/
MaybeLocal<Object> ObjectTemplate::NewInstance(Local<Context> context)
{
    ObjectTemplateImpl *impl = V82JSC::ToImpl<ObjectTemplateImpl>(this);
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    LocalException exception(ctx->isolate);
    
    JSObjectRef instance = 0;
    if (impl->m_constructor_template) {
        MaybeLocal<Function> ctor = _local<FunctionTemplate>(impl->m_constructor_template).toLocal()->GetFunction(context);
        if (!ctor.IsEmpty()) {
            JSValueRef ctor_func = V82JSC::ToJSValueRef(ctor.ToLocalChecked(), context);
            instance = JSObjectCallAsConstructor(ctx->m_context, (JSObjectRef)ctor_func, 0, 0, &exception);
        }
    } else {
        instance = JSObjectMake(ctx->m_context, nullptr, nullptr);
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
    assert(argumentCount > 0);
    JSValueRef excp = 0;
    JSObjectRef target = (JSObjectRef) arguments[0];
    JSStringRef propertyName;
    if (argumentCount > 1) {
        propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
        assert(excp==0);
    } else {
        propertyName = JSStringCreateWithUTF8CString("NONE");
    }
    size_t size = JSStringGetMaximumUTF8CStringSize(propertyName);
    char property[size];
    JSStringGetUTF8CString(propertyName, property, size);
    char *p = nullptr;
    int index = strtod(property, &p);
    if (p && (!strcmp(p, "constructor") || !strcmp(p, "__proto__") || !strcmp(p, "__private__"))) {
        return NULL;
    }

    JSValueRef value;
    if (argumentCount > 2) {
        value = arguments[2];
    } else {
        value = JSValueMakeUndefined(ctx);
    }
    JSStringRef sprivate = JSStringCreateWithUTF8CString("__private__");
    JSObjectRef __private__ = (JSObjectRef) JSObjectGetProperty(ctx, target, sprivate, &excp);
    assert(excp==0);
    TemplateWrap* wrap = reinterpret_cast<TemplateWrap*>(JSObjectGetPrivate(__private__));
    const ObjectTemplateImpl *templ = reinterpret_cast<const ObjectTemplateImpl*>(wrap->m_template);
    
    Local<Value> data;
    if (p!=nullptr) { /* Is named */
        data = ValueImpl::New(wrap->m_context, templ->m_named_data);
    } else { /* Is Indexed */
        data = ValueImpl::New(wrap->m_context, templ->m_indexed_data);
    }
    
    const ObjectTemplateImpl *impl = reinterpret_cast<const ObjectTemplateImpl*>(wrap->m_template);
    
    Local<Value> thiz = ValueImpl::New(wrap->m_context, target);
    
    v8::internal::Object * implicit[] = {
        0 /*FIXME*/,                                         // kShouldThrowOnErrorIndex = 0;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 1;
        O(wrap->m_context->isolate),                         // kIsolateIndex = 2;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueDefaultValueIndex = 3;
        O(wrap->m_context->isolate->i.roots.the_hole_value), // kReturnValueIndex = 4;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 5;
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kThisIndex = 6;
    };
    
    PropertyCallbackImpl<V> info(implicit);
    Local<Value> set = ValueImpl::New(wrap->m_context, value);
    
    if (p!=nullptr) {
        named_handler(impl,
                      ValueImpl::New(V82JSC::ToIsolate(wrap->m_context->isolate), propertyName).As<Name>(),
                      set, info);
    } else {
        indexed_handler(impl, index, set, info);
    }
    
    if (implicit[4] == O(wrap->m_context->isolate->i.roots.the_hole_value)) {
        return NULL;
    }
    
    Local<Value> retVal = info.GetReturnValue().Get();
    return V82JSC::ToJSValueRef<Value>(retVal, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
}

v8::MaybeLocal<v8::Object> ObjectTemplateImpl::NewInstance(v8::Local<v8::Context> context, JSObjectRef root)
{
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    InstanceWrap *wrap = new InstanceWrap();
    wrap->m_object_template = this;
    wrap->m_context = ctx;
    LocalException exception(ctx->isolate);
    
    // Structure:
    //
    // proxy -----> root . __private__ -->  lifecycle_object(wrap) --> TemplateWrap*
    
    // Create lifecycle object
    JSClassDefinition def = kJSClassDefinitionEmpty;
    def.attributes = kJSClassAttributeNoAutomaticPrototype;
    def.finalize = [](JSObjectRef object) {
        
    };
    JSClassRef klass = JSClassCreate(&def);
    JSObjectRef lifecycle_object = JSObjectMake(ctx->m_context, klass, (void*)wrap);
    JSClassRelease(klass);
    JSStringRef __private__ = JSStringCreateWithUTF8CString("__private__");
    JSValueRef excp = 0;
    JSObjectSetProperty(ctx->m_context, root, __private__, lifecycle_object,
                        kJSPropertyAttributeDontEnum/*|kJSPropertyAttributeReadOnly*/, &excp);
    assert(excp==0);
    
    // Create proxy
    JSObjectRef handler = 0;
    if (m_need_proxy) {
        handler = JSObjectMake(ctx->m_context, nullptr, nullptr);
        auto handler_func = [ctx, handler](const char *name, JSObjectCallAsFunctionCallback callback) -> void {
            JSValueRef excp = 0;
            JSStringRef sname = JSStringCreateWithUTF8CString(name);
            JSObjectRef f = JSObjectMakeFunctionWithCallback(ctx->m_context, sname, callback);
            JSObjectSetProperty(ctx->m_context, handler, sname, f, 0, &excp);
            JSStringRelease(sname);
            assert(excp==0);
        };
        
        handler_func("apply", [](CALLBACK_PARAMS) -> JSValueRef
        {
            return NULL;
        });
        /*if (m_named_handler.getter || m_indexed_handler.getter)*/ handler_func("get", [](CALLBACK_PARAMS) -> JSValueRef
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
            if (ret == NULL) {
                // Not handled.  Pass thru.
                assert(argumentCount>1);
                JSValueRef excp = 0;
                JSStringRef propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
                if (excp == 0) {
                    ret = JSObjectGetProperty(ctx, (JSObjectRef) arguments[0], propertyName, exception);
                } else if (exception) *exception = excp;
                JSStringRelease(propertyName);
                return ret;
            }
            return ret;
        });
        if (m_named_handler.setter || m_indexed_handler.setter) handler_func("set", [](CALLBACK_PARAMS) -> JSValueRef
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
            if (ret == NULL) {
                assert(argumentCount>2);
                return V82JSC::exec(ctx, "return _1[_2] = _3", 3, arguments);
                /*
                JSValueRef excp = 0;
                JSStringRef propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
                if (excp == 0) {
                    JSObjectSetProperty(ctx, (JSObjectRef) arguments[0], propertyName, arguments[2], kJSPropertyAttributeNone, exception);
                } else if (exception) *exception = excp;
                JSStringRelease(propertyName);
                */
            }
            return JSValueMakeBoolean(ctx, true);
        });
        /*if (m_named_handler.query || m_indexed_handler.query)*/ handler_func("has", [](CALLBACK_PARAMS) -> JSValueRef
        {
            JSValueRef ret = PropertyHandler<Integer>(PASS,
            [](const ObjectTemplateImpl* impl, Local<Name> property, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_named_handler.query) {
                    impl->m_named_handler.query(property, info);
                }
            },
            [](const ObjectTemplateImpl* impl, uint32_t index, Local<Value> value, PropertyCallbackInfo<Integer>& info)
            {
                if (impl->m_indexed_handler.query) {
                    impl->m_indexed_handler.query(index, info);
                }
            });
            if (ret == NULL) {
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "return _1.hasOwnProperty(_2)", 2, arguments);
    /*
                JSValueRef excp = 0;
                JSStringRef propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
                if (excp == 0) {
                    ret = JSValueMakeBoolean(ctx, JSObjectHasProperty(ctx, (JSObjectRef) arguments[0], propertyName));
                } else if (exception) *exception = excp;
                JSStringRelease(propertyName);
    */
            }
            return ret;
        });
        if (m_named_handler.deleter || m_indexed_handler.deleter) handler_func("deleteProperty", [](CALLBACK_PARAMS) -> JSValueRef
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
            if (ret == NULL) {
                assert(argumentCount>1);
                return V82JSC::exec(ctx, "delete _1[_2]", 2, arguments);
                /*
                JSValueRef excp = 0;
                JSStringRef propertyName = JSValueToStringCopy(ctx, arguments[1], &excp);
                if (excp == 0) {
                    ret = JSValueMakeBoolean(ctx, JSObjectDeleteProperty(ctx, (JSObjectRef) arguments[0], propertyName, exception));
                } else if (exception) *exception = excp;
                JSStringRelease(propertyName);
                */
            }
            return ret;
        });
        /*if (m_named_handler.enumerator || m_indexed_handler.enumerator)*/ handler_func("ownKeys", [](CALLBACK_PARAMS) -> JSValueRef
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
            if (ret == NULL) {
                assert(argumentCount>0);
                return V82JSC::exec(ctx, "return Object.getOwnPropertyNames(_1)", 1, arguments);
                /*
                JSPropertyNameArrayRef names = JSObjectCopyPropertyNames(ctx, (JSObjectRef)arguments[0]);
                size_t size = JSPropertyNameArrayGetCount(names);
                JSValueRef names_values[size];
                for (size_t i=0; i<size; i++) {
                    names_values[i] = JSValueMakeString(ctx, JSPropertyNameArrayGetNameAtIndex(names, i));
                }
                ret = JSObjectMakeArray(ctx, size, names_values, exception);
                JSPropertyNameArrayRelease(names);
                */
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
            return V82JSC::exec(ctx, "return Object.getOwnPropertyDescriptor(_1, _2)", 2, arguments);
        });
    }

    MaybeLocal<Object> instance;
    if (m_constructor_template) {
        instance = InitInstance(context, root, exception, m_constructor_template);
    } else {
        instance = InitInstance(context, root, exception);
    }
    if (instance.IsEmpty()) {
        return instance;
    }
    if (m_need_proxy) {
        JSValueRef args[] = {root, handler};
        return ValueImpl::New(ctx, V82JSC::exec(ctx->m_context, "return new Proxy(_1, _2)", 2, args)).As<Object>();
    }
    
    return ValueImpl::New(ctx, root).As<Object>();
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
    ObjectTemplateImpl *this_ = V82JSC::ToImpl<ObjectTemplateImpl,ObjectTemplate>(this);
    ValueImpl *name_ = V82JSC::ToImpl<ValueImpl>(name);
    JSStringRef s = JSValueToStringCopy(name_->m_context->m_context, name_->m_value, 0);
    // FIXME: Deal with attributes
    // FIXME: Deal with AccessControl
    // FIXME: Deal with signature
    
    ObjAccessor accessor;
    accessor.m_getter = getter;
    accessor.m_setter = setter;
    if (!*data) {
        data = Undefined(Isolate::GetCurrent());
    }
    accessor.m_data = V82JSC::ToJSValueRef<Value>(data, Isolate::GetCurrent());
    JSValueProtect(V82JSC::ToIsolateImpl(Isolate::GetCurrent())->m_defaultContext->m_context, accessor.m_data);
    for (auto i = this_->m_obj_accessors.begin(); i != this_->m_obj_accessors.end(); ++i ) {
        if (JSStringIsEqual(i->first, s)) {
            i->second = accessor;
            JSStringRelease(s);
            return;
        }
    }
    this_->m_obj_accessors[s] = accessor;
}
void ObjectTemplate::SetAccessor(
                 Local<Name> name, AccessorNameGetterCallback getter,
                 AccessorNameSetterCallback setter, Local<Value> data,
                 AccessControl settings, PropertyAttribute attribute,
                 Local<AccessorSignature> signature)
{
    assert(0);
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
    
}

/**
 * Gets the number of internal fields for objects generated from
 * this template.
 */
int ObjectTemplate::InternalFieldCount()
{
    return 0;
}

/**
 * Sets the number of internal fields for objects generated from
 * this template.
 */
void ObjectTemplate::SetInternalFieldCount(int value)
{
    
}

/**
 * Returns true if the object will be an immutable prototype exotic object.
 */
bool ObjectTemplate::IsImmutableProto()
{
    return false;
}

/**
 * Makes the ObjectTempate for an immutable prototype exotic object, with an
 * immutable __proto__.
 */
void ObjectTemplate::SetImmutableProto()
{
    
}

