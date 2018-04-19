//
//  Object.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Maybe<bool> Object::Set(Local<Context> context, Local<Value> key, Local<Value> value)
{
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    LocalException exception(ctx->isolate);
    JSValueRef keyv = V82JSC::ToJSValueRef(key, context);
    JSStringRef key_ = JSValueToStringCopy(ctx->m_context, keyv, &exception);
    JSValueRef value_ = V82JSC::ToJSValueRef(value, context);
    
    JSObjectSetProperty(ctx->m_context, obj, key_, value_, kJSPropertyAttributeNone, &exception);
    JSStringRelease(key_);
    
    _maybe<bool> out;
    out.has_value_ = !exception.ShouldThow();
    out.value_ = !exception.ShouldThow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Set(Local<Context> context, uint32_t index,
                                      Local<Value> value)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);

    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    JSValueRef value_ = V82JSC::ToJSValueRef(value, context);

    JSValueRef exception = nullptr;
    JSObjectSetProperty(ctx->m_context, obj, index_, value_, kJSPropertyAttributeNone, &exception);
    JSStringRelease(index_);
    
    _maybe<bool> out;
    out.has_value_ = true;
    out.value_ = exception == nullptr;
    
    return out.toMaybe();
}

// Implements CreateDataProperty (ECMA-262, 7.3.4).
//
// Defines a configurable, writable, enumerable property with the given value
// on the object unless the property already exists and is not configurable
// or the object is not extensible.
//
// Returns true on success.
Maybe<bool> Object::CreateDataProperty(Local<Context> context,
                               Local<Name> key,
                               Local<Value> value)
{
    return Nothing<bool>();
}

Maybe<bool> Object::CreateDataProperty(Local<Context> context,
                                       uint32_t index,
                                       Local<Value> value)
{
    return Nothing<bool>();
}

// Implements DefineOwnProperty.
//
// In general, CreateDataProperty will be faster, however, does not allow
// for specifying attributes.
//
// Returns true on success.
Maybe<bool> Object::DefineOwnProperty(
                                      Local<Context> context, Local<Name> key, Local<Value> value,
                                      PropertyAttribute attributes)
{
    return Nothing<bool>();
}

// Implements Object.DefineProperty(O, P, Attributes), see Ecma-262 19.1.2.4.
//
// The defineProperty function is used to add an own property or
// update the attributes of an existing own property of an object.
//
// Both data and accessor descriptors can be used.
//
// In general, CreateDataProperty is faster, however, does not allow
// for specifying attributes or an accessor descriptor.
//
// The PropertyDescriptor can change when redefining a property.
//
// Returns true on success.
Maybe<bool> Object::DefineProperty(Local<Context> context, Local<Name> key,
                           PropertyDescriptor& descriptor)
{
    return Nothing<bool>();
}

// Sets an own property on this object bypassing interceptors and
// overriding accessors or read-only properties.
//
// Note that if the object has an interceptor the property will be set
// locally, but since the interceptor takes precedence the local property
// will only be returned if the interceptor doesn't return a value.
//
// Note also that this only works for named properties.
MaybeLocal<Value> Object::Get(Local<Context> context, Local<Value> key)
{
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);

    LocalException exception(ctx->isolate);
    JSValueRef keyv = V82JSC::ToJSValueRef(key, context);
    JSStringRef key_ = JSValueToStringCopy(ctx->m_context, keyv, &exception);

    JSValueRef prop = JSObjectGetProperty(ctx->m_context, obj, key_, &exception);
    JSStringRelease(key_);

    if (!exception.ShouldThow()) {
        return MaybeLocal<Value>(ValueImpl::New(ctx, prop));
    }
    
    return MaybeLocal<Value>();
}

MaybeLocal<Value> Object::Get(Local<Context> context, uint32_t index)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);

    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);

    LocalException exception(ctx->isolate);
    JSValueRef prop = JSObjectGetProperty(ctx->m_context, obj, index_, &exception);
    JSStringRelease(index_);
    if (!exception.ShouldThow()) {
        return MaybeLocal<Value>(ValueImpl::New(ctx, prop));
    }
    
    return MaybeLocal<Value>();
}

/**
 * Gets the property attributes of a property which can be None or
 * any combination of ReadOnly, DontEnum and DontDelete. Returns
 * None when the property doesn't exist.
 */
Maybe<PropertyAttribute> Object::GetPropertyAttributes(Local<Context> context, Local<Value> key)
{
    return Nothing<PropertyAttribute>();
}

/**
 * Returns Object.getOwnPropertyDescriptor as per ES2016 section 19.1.2.6.
 */
MaybeLocal<Value> Object::GetOwnPropertyDescriptor(Local<Context> context, Local<Name> key)
{
    return MaybeLocal<Value>();
}

/**
 * Object::Has() calls the abstract operation HasProperty(O, P) described
 * in ECMA-262, 7.3.10. Has() returns
 * true, if the object has the property, either own or on the prototype chain.
 * Interceptors, i.e., PropertyQueryCallbacks, are called if present.
 *
 * Has() has the same side effects as JavaScript's `variable in object`.
 * For example, calling Has() on a revoked proxy will throw an exception.
 *
 * \note Has() converts the key to a name, which possibly calls back into
 * JavaScript.
 *
 * See also v8::Object::HasOwnProperty() and
 * v8::Object::HasRealNamedProperty().
 */
Maybe<bool> Object::Has(Local<Context> context, Local<Value> key)
{
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);

    LocalException exception(ctx->isolate);
    JSValueRef keyv = V82JSC::ToJSValueRef(key, context);
    JSStringRef key_ = JSValueToStringCopy(ctx->m_context, keyv, &exception);

    _maybe<bool> out;
    out.has_value_ = !exception.ShouldThow();
    out.value_ = JSObjectHasProperty(ctx->m_context, obj, key_);
    JSStringRelease(key_);
    return out.toMaybe();
}

Maybe<bool> Object::Delete(Local<Context> context, Local<Value> key)
{
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    LocalException exception(ctx->isolate);
    JSValueRef keyv = V82JSC::ToJSValueRef(key, context);
    JSStringRef key_ = JSValueToStringCopy(ctx->m_context, keyv, &exception);
    
    _maybe<bool> out;
    if (!exception.ShouldThow()) {
        out.value_ = JSObjectDeleteProperty(ctx->m_context, obj, key_, &exception);
    }
    out.has_value_ = !exception.ShouldThow();
    JSStringRelease(key_);
    return out.toMaybe();
}

Maybe<bool> Object::Has(Local<Context> context, uint32_t index)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);

    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);

    _maybe<bool> out;
    out.has_value_ = true;
    out.value_ = JSObjectHasProperty(ctx->m_context, obj, index_);
    JSStringRelease(index_);
    return out.toMaybe();
}

Maybe<bool> Object::Delete(Local<Context> context, uint32_t index)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);
    
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    ContextImpl *ctx = V82JSC::ToContextImpl(context);

    _maybe<bool> out;
    JSValueRef exception = nullptr;
    out.value_ = JSObjectDeleteProperty(ctx->m_context, obj, index_, &exception);
    JSStringRelease(index_);
    out.has_value_ = exception == nullptr;
    return out.toMaybe();
}

Maybe<bool> Object::SetAccessor(Local<Context> context,
                                Local<Name> name,
                                AccessorNameGetterCallback getter,
                                AccessorNameSetterCallback setter,
                                MaybeLocal<Value> data,
                                AccessControl settings,
                                PropertyAttribute attribute)
{
    return Nothing<bool>();
}

void Object::SetAccessorProperty(Local<Name> name, Local<Function> getter,
                                 Local<Function> setter,
                                 PropertyAttribute attribute,
                                 AccessControl settings)
{
    
}

/**
 * Sets a native data property like Template::SetNativeDataProperty, but
 * this method sets on this object directly.
 */
Maybe<bool> Object::SetNativeDataProperty(Local<Context> context, Local<Name> name,
                                          AccessorNameGetterCallback getter,
                                          AccessorNameSetterCallback setter,
                                          Local<Value> data, PropertyAttribute attributes)
{
    return Nothing<bool>();
}

/**
 * Functionality for private properties.
 * This is an experimental feature, use at your own risk.
 * Note: Private properties are not inherited. Do not rely on this, since it
 * may change.
 */
Maybe<bool> Object::HasPrivate(Local<Context> context, Local<Private> key)
{
    return Nothing<bool>();
}
Maybe<bool> Object::SetPrivate(Local<Context> context, Local<Private> key,
                               Local<Value> value)
{
    return Nothing<bool>();
}
Maybe<bool> Object::DeletePrivate(Local<Context> context, Local<Private> key)
{
    return Nothing<bool>();
}
MaybeLocal<Value> Object::GetPrivate(Local<Context> context, Local<Private> key)
{
    return MaybeLocal<Value>();
}

/**
 * Returns an array containing the names of the enumerable properties
 * of this object, including properties from prototype objects.  The
 * array returned by this method contains the same values as would
 * be enumerated by a for-in statement over this object.
 */
MaybeLocal<Array> Object::GetPropertyNames(Local<Context> context)
{
    return MaybeLocal<Array>();
}
MaybeLocal<Array> Object::GetPropertyNames(Local<Context> context, KeyCollectionMode mode,
                                           PropertyFilter property_filter, IndexFilter index_filter)
{
    return MaybeLocal<Array>();
}

/**
 * This function has the same functionality as GetPropertyNames but
 * the returned array doesn't contain the names of properties from
 * prototype objects.
 */
MaybeLocal<Array> Object::GetOwnPropertyNames(Local<Context> context)
{
    return MaybeLocal<Array>();
}

/**
 * Returns an array containing the names of the filtered properties
 * of this object, including properties from prototype objects.  The
 * array returned by this method contains the same values as would
 * be enumerated by a for-in statement over this object.
 */
MaybeLocal<Array> Object::GetOwnPropertyNames(Local<Context> context, PropertyFilter filter)
{
    return MaybeLocal<Array>();
}

/**
 * Get the prototype object.  This does not skip objects marked to
 * be skipped by __proto__ and it does not consult the security
 * handler.
 */
Local<Value> Object::GetPrototype()
{
    ContextImpl* ctximpl = V82JSC::ToContextImpl<Object>(this);
    JSContextRef ctx = ctximpl->m_context;
    JSValueRef obj = V82JSC::ToJSValueRef<Value>(this, _local<Context>(ctximpl).toLocal());
    JSValueRef proto = V82JSC::exec(ctx, "return Object.getPrototypeOf(_1)", 1, &obj);
    return ValueImpl::New(ctximpl, proto);
}

/**
 * Set the prototype object.  This does not skip objects marked to
 * be skipped by __proto__ and it does not consult the security
 * handler.
 */
Maybe<bool> Object::SetPrototype(Local<Context> context,
                                 Local<Value> prototype)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);
    JSValueRef proto = V82JSC::ToJSValueRef<Value>(prototype, context);
    
    JSObjectSetPrototype(ctx, obj, proto);
    return _maybe<bool>(true).toMaybe();
}

/**
 * Finds an instance of the given function template in the prototype
 * chain.
 */
Local<Object> Object::FindInstanceInPrototypeChain(Local<FunctionTemplate> tmpl)
{
    ContextImpl* ctximpl = V82JSC::ToContextImpl<Object>(this);
    FunctionTemplateImpl* tmplimpl = V82JSC::ToImpl<FunctionTemplateImpl>(tmpl);
    Local<Context> context = _local<Context>(ctximpl).toLocal();
    
    JSStringRef sprivate = JSStringCreateWithUTF8CString("__private__");
    Local<Value> proto = _local<Value>(this).toLocal();
    while (proto->IsObject()) {
        JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef(proto, context);
        JSValueRef excp=0;
        JSObjectRef __private__ = (JSObjectRef) JSObjectGetProperty(ctximpl->m_context, obj, sprivate, &excp);
        assert(excp==0);
        bool hasOwn = JSValueToBoolean(ctximpl->m_context,
                                       V82JSC::exec(ctximpl->m_context, "return !_1.__proto__ || (_1.__proto__.__private__ !== _1.__private__)", 1, &obj));
        if (hasOwn && JSValueIsObject(ctximpl->m_context, __private__)) {
            InstanceWrap *instance_wrap = (InstanceWrap*) JSObjectGetPrivate(__private__);
            if(instance_wrap) {
                for (const TemplateImpl *t = instance_wrap->m_object_template->m_constructor_template; t; t = t->m_parent) {
                    if (t == tmplimpl) {
                        return proto.As<Object>();
                    }
                }
            }
        }
        proto = proto.As<Object>()->GetPrototype();
    }
    JSStringRelease(sprivate);

    return Local<Object>();
}

/**
 * Call builtin Object.prototype.toString on this object.
 * This is different from Value::ToString() that may call
 * user-defined toString function. This one does not.
 */
MaybeLocal<String> Object::ObjectProtoToString(Local<Context> context)
{
    return MaybeLocal<String>();
}

/**
 * Returns the name of the function invoked as a constructor for this object.
 */
Local<String> Object::GetConstructorName()
{
    ValueImpl* obj = V82JSC::ToImpl<ValueImpl,Object>(this);
    if (obj) {
        JSStringRef ctor = JSStringCreateWithUTF8CString("constructor");
        JSValueRef excp = nullptr;
        JSValueRef vctor = JSObjectGetProperty(obj->m_context->m_context, (JSObjectRef) obj->m_value, ctor, &excp);
        JSStringRelease(ctor);
        assert(excp==nullptr);
        if (JSValueIsObject(obj->m_context->m_context, vctor)) {
            JSStringRef name = JSStringCreateWithUTF8CString("name");
            JSValueRef vname = JSObjectGetProperty(obj->m_context->m_context, (JSObjectRef) vctor, name, &excp);
            JSStringRelease(name);
            assert(excp==nullptr);
            return ValueImpl::New(obj->m_context, vname)->ToString(_local<Context>(obj->m_context).toLocal()).ToLocalChecked();
        }
    }

    return Local<String>(nullptr);
}

/**
 * Sets the integrity level of the object.
 */
Maybe<bool> Object::SetIntegrityLevel(Local<Context> context, IntegrityLevel level)
{
    return Nothing<bool>();
}

/** Gets the number of internal fields for this Object. */
int Object::InternalFieldCount()
{
    return 0;
}

/** Sets the value in an internal field. */
void Object::SetInternalField(int index, Local<Value> value)
{
    
}

/**
 * Sets a 2-byte-aligned native pointer in an internal field. To retrieve such
 * a field, GetAlignedPointerFromInternalField must be used, everything else
 * leads to undefined behavior.
 */
void Object::SetAlignedPointerInInternalField(int index, void* value)
{
    
}
void Object::SetAlignedPointerInInternalFields(int argc, int indices[],
                                       void* values[])
{
    
}

// Testers for local properties.

/**
 * HasOwnProperty() is like JavaScript's Object.prototype.hasOwnProperty().
 *
 * See also v8::Object::Has() and v8::Object::HasRealNamedProperty().
 */
Maybe<bool> Object::HasOwnProperty(Local<Context> context, Local<Name> key)
{
    return Nothing<bool>();
}
Maybe<bool> Object::HasOwnProperty(Local<Context> context, uint32_t index)
{
    return Nothing<bool>();
}

/**
 * Use HasRealNamedProperty() if you want to check if an object has an own
 * property without causing side effects, i.e., without calling interceptors.
 *
 * This function is similar to v8::Object::HasOwnProperty(), but it does not
 * call interceptors.
 *
 * \note Consider using non-masking interceptors, i.e., the interceptors are
 * not called if the receiver has the real named property. See
 * `v8::PropertyHandlerFlags::kNonMasking`.
 *
 * See also v8::Object::Has().
 */
Maybe<bool> Object::HasRealNamedProperty(Local<Context> context, Local<Name> key)
{
    return Nothing<bool>();
}
Maybe<bool> Object::HasRealIndexedProperty(Local<Context> context, uint32_t index)
{
    return Nothing<bool>();
}
Maybe<bool> Object::HasRealNamedCallbackProperty(Local<Context> context, Local<Name> key)
{
    return Nothing<bool>();
}

/**
 * If result.IsEmpty() no real property was located in the prototype chain.
 * This means interceptors in the prototype chain are not called.
 */
MaybeLocal<Value> Object::GetRealNamedPropertyInPrototypeChain(Local<Context> context,
                                                               Local<Name> key)
{
    return MaybeLocal<Value>();
}

/**
 * Gets the property attributes of a real property in the prototype chain,
 * which can be None or any combination of ReadOnly, DontEnum and DontDelete.
 * Interceptors in the prototype chain are not called.
 */
Maybe<PropertyAttribute>
Object::GetRealNamedPropertyAttributesInPrototypeChain(Local<Context> context,
                                                       Local<Name> key)
{
    return Nothing<PropertyAttribute>();
}

/**
 * If result.IsEmpty() no real property was located on the object or
 * in the prototype chain.
 * This means interceptors in the prototype chain are not called.
 */
MaybeLocal<Value> Object::GetRealNamedProperty(Local<Context> context, Local<Name> key)
{
    return MaybeLocal<Value>();
}

/**
 * Gets the property attributes of a real property which can be
 * None or any combination of ReadOnly, DontEnum and DontDelete.
 * Interceptors in the prototype chain are not called.
 */
Maybe<PropertyAttribute> Object::GetRealNamedPropertyAttributes(Local<Context> context, Local<Name> key)
{
    return Nothing<PropertyAttribute>();
}

/** Tests for a named lookup interceptor.*/
bool Object::HasNamedLookupInterceptor()
{
    return false;
}

/** Tests for an index lookup interceptor.*/
bool Object::HasIndexedLookupInterceptor()
{
    return false;
}

/**
 * Returns the identity hash for this object. The current implementation
 * uses a hidden property on the object to store the identity hash.
 *
 * The return value will never be 0. Also, it is not guaranteed to be
 * unique.
 */
int Object::GetIdentityHash()
{
    return 1;
}

/**
 * Clone this object with a fast but shallow copy.  Values will point
 * to the same values as the original object.
 */
// TODO(dcarney): take an isolate and optionally bail out?
Local<Object> Object::Clone()
{
    return Local<Object>();
}

/**
 * Returns the context in which the object was created.
 */
Local<Context> Object::CreationContext()
{
    return Local<Context>();
}

/**
 * Checks whether a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 * When an Object is callable this method returns true.
 */
bool Object::IsCallable()
{
    return false;
}

/**
 * True if this object is a constructor.
 */
bool Object::IsConstructor()
{
    return false;
}

/**
 * Call an Object as a function if a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 */
MaybeLocal<Value> Object::CallAsFunction(Local<Context> context,
                                         Local<Value> recv,
                                         int argc,
                                         Local<Value> argv[])
{
    return MaybeLocal<Value>();
}

/**
 * Call an Object as a constructor if a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 * Note: This method behaves like the Function::NewInstance method.
 */
MaybeLocal<Value> Object::CallAsConstructor(Local<Context> context,
                                            int argc, Local<Value> argv[])
{
    return MaybeLocal<Value>();
}

Local<Object> Object::New(Isolate* isolate)
{
    IsolateImpl *i = reinterpret_cast<IsolateImpl*>(isolate);
    Local<Value> o = ValueImpl::New(i->m_defaultContext, JSObjectMake(i->m_defaultContext->m_context, 0, 0));
    _local<Object> obj(*o);
    return obj.toLocal();
}

void* Object::SlowGetAlignedPointerFromInternalField(int index)
{
    return nullptr;
}

Local<Value> Object::SlowGetInternalField(int index)
{
    return Local<Value>();
}

