//
//  ObjectTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/** Creates an ObjectTemplate. */
Local<ObjectTemplate> ObjectTemplate::New(
                                 Isolate* isolate,
                                 Local<FunctionTemplate> constructor)
{
    return Local<ObjectTemplate>();
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
    ObjectTemplateImpl *impl = reinterpret_cast<ObjectTemplateImpl*>(this);
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    impl->m_class = JSClassCreate(&impl->m_definition);
    
    ObjectTemplateWrap *wrap = new ObjectTemplateWrap();
    wrap->m_template = impl;
    wrap->m_context = ctx;
    JSValueRef exception = nullptr;

    JSObjectRef instance = JSObjectMake(ctx->m_context, impl->m_class, (void*)wrap);
    
    if (impl->m_parent) {
        MaybeLocal<Object> proto = reinterpret_cast<ObjectTemplate*>(impl->m_parent)->NewInstance(context);
        if (!proto.IsEmpty()) {
            JSObjectSetPrototype(ctx->m_context, instance, V82JSC::ToJSValueRef<Object>(proto.ToLocalChecked(), context));
            
            JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
            JSObjectSetProperty(ctx->m_context, instance, sprototype, V82JSC::ToJSValueRef<Object>(proto.ToLocalChecked(), context), kJSPropertyAttributeNone, &exception);
            JSStringRelease(sprototype);
            assert(exception==nullptr);
        }
    }
    
    for (auto i=impl->m_properties.begin(); i!=impl->m_properties.end(); ++i) {
        typedef internal::Object O;
        typedef internal::Internals I;
        O* obj = reinterpret_cast<O* const>(i->second->pMap);
        int t = I::GetInstanceType(obj);
        JSValueRef v = i->second->m_value;
        if (t == v8::internal::FUNCTION_TEMPLATE_INFO_TYPE) {
            MaybeLocal<Object> o = reinterpret_cast<ObjectTemplate*>(i->second)->NewInstance(context);
            if (!o.IsEmpty()) {
                v = reinterpret_cast<ValueImpl*>(*o.ToLocalChecked())->m_value;
            }
        }
        JSValueRef exception = nullptr;
        JSObjectSetProperty(ctx->m_context, instance, i->first, v, kJSPropertyAttributeNone, &exception);
        assert(exception==nullptr);
    }
    
    JSStringRef getset = JSStringCreateWithUTF8CString(
           "delete __o__[__n__]; "
           "if (!__setter__) Object.defineProperty(__o__, __n__, { get: __getter__ }); "
           "else if (!__getter__) Object.defineProperty(__o__, __n__, { set: __setter__ }); "
           "else Object.defineProperty(__o__, __n__, { get: __getter__, set: __setter__ });");
    JSStringRef name = JSStringCreateWithUTF8CString("getset");
    JSStringRef paramNames[] = {
        JSStringCreateWithUTF8CString("__o__"),
        JSStringCreateWithUTF8CString("__n__"),
        JSStringCreateWithUTF8CString("__getter__"),
        JSStringCreateWithUTF8CString("__setter__"),
    };
    JSObjectRef getsetF = JSObjectMakeFunction(ctx->m_context,
                                               name,
                                               sizeof paramNames / sizeof (JSStringRef),
                                               paramNames,
                                               getset, 0, 0, &exception);
    assert(exception==nullptr);
    
    for (auto i=impl->m_property_accessors.begin(); i!=impl->m_property_accessors.end(); ++i) {
        Local<ObjectTemplate> getter = _local<ObjectTemplate>(i->second.m_getter).toLocal();
        Local<ObjectTemplate> setter = _local<ObjectTemplate>(i->second.m_setter).toLocal();
        if (getter.IsEmpty() && setter.IsEmpty()) continue;
        
        JSValueRef params[] = {
            instance,
            JSValueMakeString(ctx->m_context, i->first),
            0,
            0
        };
        if (!getter.IsEmpty()) {
            params[2] = V82JSC::ToJSValueRef<Object>(getter->NewInstance(context).ToLocalChecked(), context);
        }
        if (!setter.IsEmpty()) {
            params[3] = V82JSC::ToJSValueRef<Object>(setter->NewInstance(context).ToLocalChecked(), context);
        }
        JSObjectCallAsFunction(ctx->m_context,
                               getsetF,
                               0,
                               sizeof params / sizeof (JSValueRef),
                               params,
                               &exception);
        assert(exception==nullptr);
    }

    for (auto i=impl->m_obj_accessors.begin(); i!=impl->m_obj_accessors.end(); ++i) {
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
            def.callAsFunction = ObjectTemplateImpl::objectGetterCallback;
            JSClassRef claz = JSClassCreate(&def);
            getter = JSObjectMake(ctx->m_context, claz, priv);
            JSClassRelease(claz);
        }
        JSObjectRef setter = 0;
        if (priv->m_setter) {
            def.callAsFunction = ObjectTemplateImpl::objectSetterCallback;
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
                               &exception);
        assert(exception==nullptr);
    }

    JSStringRelease(getset);
    JSStringRelease(name);
    for (int x=0; x<sizeof paramNames / sizeof(JSStringRef); x++) JSStringRelease(paramNames[x]);

    return _local<Object>(*ValueImpl::New(ctx, instance)).toLocal();
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

JSValueRef ObjectTemplateImpl::objectGetterCallback(JSContextRef ctx,
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
    
    PropertyCallbackImpl<Value> info(implicit);
    
    wrap->m_getter(ValueImpl::New(wrap->m_context, wrap->m_property).As<String>(), info);
    
    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
}

JSValueRef ObjectTemplateImpl::objectSetterCallback(JSContextRef ctx,
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
    
    wrap->m_setter(ValueImpl::New(wrap->m_context, wrap->m_property).As<String>(), value, info);
    
    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
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
    ObjectTemplateImpl *this_ = static_cast<ObjectTemplateImpl*>(reinterpret_cast<ValueImpl*>(this));
    ValueImpl *name_ = reinterpret_cast<ValueImpl*>(*name);
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
        if (JSStringIsEqual(i->first, name_->m_string)) {
            i->second = accessor;
            return;
        }
    }
    this_->m_obj_accessors[name_->m_string] = accessor;
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
