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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
        V82JSC::ToJSValueRef(key, context),
        V82JSC::ToJSValueRef(value, context)
    };
    
    JSValueRef ret = V82JSC::exec(ctx, "return _3 == (_1[_2] = _3)", 3, args, &exception);
    
    _maybe<bool> out;
    if (!exception.ShouldThow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Set(Local<Context> context, uint32_t index,
                                      Local<Value> value)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);

    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSValueRef value_ = V82JSC::ToJSValueRef(value, context);

    JSValueRef exception = nullptr;
    JSObjectSetProperty(ctx, (JSObjectRef)obj, index_, value_, kJSPropertyAttributeNone, &exception);
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
    return DefineOwnProperty(context, key, value);
}

Maybe<bool> Object::CreateDataProperty(Local<Context> context,
                                       uint32_t index,
                                       Local<Value> value)
{
    Local<Value> k = Number::New(V82JSC::ToIsolate(V82JSC::ToContextImpl(context)), index);
    Local<Name> key = k->ToString(context).ToLocalChecked();
    return CreateDataProperty(context, key, value);
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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSValueRef key_ = V82JSC::ToJSValueRef(key, context);
    JSValueRef v = V82JSC::ToJSValueRef(value, context);

    JSValueRef args[] = {
        obj,
        key_,
        v,
        JSValueMakeNumber(ctx, attributes)
    };
    
    /* None = 0,
     ReadOnly = 1 << 0,
     DontEnum = 1 << 1,
     DontDelete = 1 << 2
     */
    TryCatch try_catch(V82JSC::ToIsolate(this));
    {
        LocalException exception(V82JSC::ToIsolateImpl(this));
        V82JSC::exec(ctx,
                     "Object.defineProperty(_1, _2, "
                     "{ writable : !(_4&(1<<0)), "
                     "  enumerable : !(_4&(1<<1)), "
                     "  configurable : !(_4&(1<<2)), "
                     "  value: _3 })", 4, args, &exception);
    }
    if (try_catch.HasCaught()) return _maybe<bool>(false).toMaybe();

    return _maybe<bool>(true).toMaybe();
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
    assert(0);
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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
        V82JSC::ToJSValueRef(key, context)
    };
    
    JSValueRef ret = V82JSC::exec(ctx, "return _1[_2]", 2, args, &exception);
    
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), ret);
    }
    return Local<Value>();
}

MaybeLocal<Value> Object::Get(Local<Context> context, uint32_t index)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);

    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef prop = JSObjectGetProperty(ctx, (JSObjectRef)obj, index_, &exception);
    JSStringRelease(index_);
    if (!exception.ShouldThow()) {
        return MaybeLocal<Value>(ValueImpl::New(V82JSC::ToContextImpl(context), prop));
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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
        V82JSC::ToJSValueRef(key, context),
    };
    
    JSValueRef ret = V82JSC::exec(ctx,
                                  "const None = 0, ReadOnly = 1 << 0, DontEnum = 1 << 1, DontDelete = 1 << 2; "
                                  "var d = Object.getOwnPropertyDescriptor(_1, _2); "
                                  "var attr = None; if (!d) return attr; "
                                  "attr |= (!d.writable) ? ReadOnly : 0; "
                                  "attr |= (!d.enumerable) ? DontEnum : 0; "
                                  "attr |= (!d.configurable) ? DontDelete : 0; "
                                  "return attr"
                                  , 2, args, &exception);

    _maybe<PropertyAttribute> out;
    if (!exception.ShouldThow()) {
        JSValueRef excp = 0;
        out.value_ = (PropertyAttribute) JSValueToNumber(ctx, ret, &excp);
        assert(excp==0);
    }
    out.has_value_ = !exception.ShouldThow();
    
    return out.toMaybe();
}

/**
 * Returns Object.getOwnPropertyDescriptor as per ES2016 section 19.1.2.6.
 */
MaybeLocal<Value> Object::GetOwnPropertyDescriptor(Local<Context> context, Local<Name> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(key, context)
    };
    LocalException exception(iso);
    JSValueRef descriptor = V82JSC::exec(ctx,
                                         "return Object.getOwnPropertyDescriptor(_1, _2)",
                                         2, args, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<Value>();
    }
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), descriptor);
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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
        V82JSC::ToJSValueRef(key, context)
    };
    
    JSValueRef ret = V82JSC::exec(ctx, "return (_2 in _1)", 2, args, &exception);
    
    _maybe<bool> out;
    if (!exception.ShouldThow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Delete(Local<Context> context, Local<Value> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
        V82JSC::ToJSValueRef(key, context)
    };

    JSValueRef ret = V82JSC::exec(ctx, "return delete _1[_2]", 2, args, &exception);

    _maybe<bool> out;
    if (!exception.ShouldThow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Has(Local<Context> context, uint32_t index)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);

    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);

    _maybe<bool> out;
    out.has_value_ = true;
    out.value_ = JSObjectHasProperty(ctx, obj, index_);
    JSStringRelease(index_);
    return out.toMaybe();
}

Maybe<bool> Object::Delete(Local<Context> context, uint32_t index)
{
    char ndx[50];
    sprintf(ndx, "%d", index);
    JSStringRef index_ = JSStringCreateWithUTF8CString(ndx);
    
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef<Object>(this, context);

    _maybe<bool> out;
    JSValueRef exception = nullptr;
    out.value_ = JSObjectDeleteProperty(ctx, obj, index_, &exception);
    JSStringRelease(index_);
    out.has_value_ = exception == nullptr;
    return out.toMaybe();
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

Maybe<bool> Object::SetAccessor(Local<Context> context,
                                Local<Name> name,
                                AccessorNameGetterCallback getter,
                                AccessorNameSetterCallback setter,
                                MaybeLocal<Value> data,
                                AccessControl settings,
                                PropertyAttribute attribute)
{
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    struct AccessorInfo {
        AccessorNameGetterCallback getter;
        AccessorNameSetterCallback setter;
        IsolateImpl *m_isolate;
        JSValueRef m_property;
        JSValueRef m_data;
        JSValueRef m_holder;
        ~AccessorInfo()
        {
            Isolate* isolate = reinterpret_cast<Isolate*>(m_isolate);
            HandleScope scope(isolate);
            
            Local<Context> context = V82JSC::OperatingContext(isolate);
            JSContextRef ctx = V82JSC::ToContextRef(context);
            if (m_property) JSValueUnprotect(ctx, m_property);
            if (m_data) JSValueUnprotect(ctx, m_data);
            // m_holder is weak
        }
    };
    
    const auto callback = [](JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                             size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception) -> JSValueRef
    {
        AccessorInfo* wrap = reinterpret_cast<AccessorInfo*>(JSObjectGetPrivate(function));
        Local<Context> context = ContextImpl::New(V82JSC::ToIsolate(wrap->m_isolate), ctx);
        ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
        
        Local<Value> thiz = ValueImpl::New(ctximpl, thisObject);
        Local<Value> data = ValueImpl::New(ctximpl, wrap->m_data);
        Local<Value> holder = ValueImpl::New(ctximpl, wrap->m_holder);
        typedef v8::internal::Heap::RootListIndex R;
        internal::Object *the_hole = wrap->m_isolate->ii.heap()->root(R::kTheHoleValueRootIndex);

        // FIXME: This doesn't work
        JSStringRef s = JSStringCreateWithUTF8CString("(function() {return !this;})()");
        bool isStrict = JSValueToBoolean(ctx, JSEvaluateScript(ctx, s, 0, 0, 0, 0));
        internal::Object *shouldThrow = internal::Smi::FromInt(isStrict?1:0);
        
        v8::internal::Object * implicit[] = {
            shouldThrow,                                             // kShouldThrowOnErrorIndex = 0;
            * reinterpret_cast<v8::internal::Object**>(*holder),     // kHolderIndex = 1;
            O(wrap->m_isolate),                                      // kIsolateIndex = 2;
            the_hole,                                                // kReturnValueDefaultValueIndex = 3;
            the_hole,                                                // kReturnValueIndex = 4;
            * reinterpret_cast<v8::internal::Object**>(*data),       // kDataIndex = 5;
            * reinterpret_cast<v8::internal::Object**>(*thiz),       // kThisIndex = 6;
        };
        
        wrap->m_isolate->ii.thread_local_top()->scheduled_exception_ = the_hole;
        TryCatch try_catch(V82JSC::ToIsolate(wrap->m_isolate));

        Local<Value> ret = Undefined(V82JSC::ToIsolate(wrap->m_isolate));
        if (argumentCount == 0) {
            PropertyCallbackImpl<Value> info(implicit);
            wrap->getter(ValueImpl::New(ctximpl, wrap->m_property).As<Name>(), info);
            ret = info.GetReturnValue().Get();
        } else {
            PropertyCallbackImpl<void> info(implicit);
            wrap->setter(ValueImpl::New(ctximpl, wrap->m_property).As<Name>(),
                         ValueImpl::New(ctximpl, arguments[0]),
                         info);
        }
        
        if (try_catch.HasCaught()) {
            *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
        } else if (wrap->m_isolate->ii.thread_local_top()->scheduled_exception_ != the_hole) {
            internal::Object * excep = wrap->m_isolate->ii.thread_local_top()->scheduled_exception_;
            if (excep->IsHeapObject()) {
                ValueImpl* i = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(excep) - internal::kHeapObjectTag);
                *exception = i->m_value;
            } else {
                *exception = JSValueMakeNumber(ctx, internal::Smi::ToInt(excep));
            }
            wrap->m_isolate->ii.thread_local_top()->scheduled_exception_ = the_hole;
        }

        return V82JSC::ToJSValueRef<Value>(ret, context);
    };
    
    AccessorInfo *wrap = new AccessorInfo();
    wrap->m_isolate = iso;
    wrap->m_property = V82JSC::ToJSValueRef(name, context);
    JSValueProtect(ctximpl->m_ctxRef, wrap->m_property);
    wrap->getter = getter;
    wrap->setter = setter;
    if (data.IsEmpty()) data = Undefined(V82JSC::ToIsolate(iso));
    wrap->m_data = V82JSC::ToJSValueRef(data.ToLocalChecked(), context);
    JSValueProtect(ctximpl->m_ctxRef, wrap->m_data);
    wrap->m_holder = V82JSC::ToJSValueRef(this, context);

    JSClassDefinition def = kJSClassDefinitionEmpty;
    def.attributes = kJSClassAttributeNoAutomaticPrototype;
    def.callAsFunction = callback;
    def.finalize = [](JSObjectRef obj)
    {
        AccessorInfo *info = (AccessorInfo*) JSObjectGetPrivate(obj);
        delete info;
    };
    JSClassRef claz = JSClassCreate(&def);
    JSObjectRef accessor_function = JSObjectMake(ctximpl->m_ctxRef, claz, wrap);
    JSClassRelease(claz);
    Local<Function> accessor = ValueImpl::New(ctximpl, accessor_function).As<Function>();
    
    TryCatch try_catch(V82JSC::ToIsolate(iso));
    
    SetAccessorProperty(name,
                        (getter) ? accessor : Local<Function>(),
                        (setter) ? accessor : Local<Function>(),
                        attribute,
                        settings);

    return _maybe<bool>(!try_catch.HasCaught()).toMaybe();
}

void Object::SetAccessorProperty(Local<Name> name, Local<Function> getter,
                                 Local<Function> setter,
                                 PropertyAttribute attribute,
                                 AccessControl settings)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    
    // FIXME: Deal with access control
    
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
        V82JSC::ToJSValueRef(name, context),
        !getter.IsEmpty() ? V82JSC::ToJSValueRef(getter, context) : 0,
        !setter.IsEmpty() ? V82JSC::ToJSValueRef(setter, context) : 0,
        JSValueMakeNumber(ctx, attribute)
    };
    
    /* None = 0,
     ReadOnly = 1 << 0, // Not used with accessors
     DontEnum = 1 << 1,
     DontDelete = 1 << 2
     */
    V82JSC::exec(ctx,
                 "delete _1[_2]; "
                 "var desc = "
                 "{ "
                 "  enumerable : !(_5&(1<<1)), "
                 "  configurable : !(_5&(1<<2))}; "
                 "if (!_4) { desc.get = _3; desc.set = function(v) { delete this[_2]; this[_2] = v; }; }"
                 "else if (!_3) { desc.set = _4; }"
                 "else { desc.get = _3; desc.set = _4; }"
                 "Object.defineProperty(_1, _2, desc);",
                 5, args, &exception);
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
    return SetAccessor(context, name, getter, setter, data, AccessControl::DEFAULT, attributes);
}

/**
 * Functionality for private properties.
 * This is an experimental feature, use at your own risk.
 * Note: Private properties are not inherited. Do not rely on this, since it
 * may change.
 */
Maybe<bool> Object::HasPrivate(Local<Context> context, Local<Private> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (wrap && wrap->m_private_properties) {
        JSValueRef args[] = {
            wrap->m_private_properties,
            V82JSC::ToJSValueRef(key, context)
        };
        LocalException exception(iso);
        JSValueRef ret = V82JSC::exec(ctx, "return _1.hasOwnProperty(_2)", 2, args);
        if (exception.ShouldThow()) return Nothing<bool>();
        return _maybe<bool>(JSValueToBoolean(ctx, ret)).toMaybe();
    }
    return _maybe<bool>(false).toMaybe();
}
Maybe<bool> Object::SetPrivate(Local<Context> context, Local<Private> key,
                               Local<Value> value)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (!wrap) wrap = V82JSC::makePrivateInstance(iso, ctx, (JSObjectRef)obj);
    if (!wrap->m_private_properties) {
        wrap->m_private_properties = JSObjectMake(ctx, 0, 0);
    }
    JSValueRef args[] = {
        wrap->m_private_properties,
        V82JSC::ToJSValueRef(key, context),
        V82JSC::ToJSValueRef(value, context)
    };
    LocalException exception(iso);
    V82JSC::exec(ctx, "_1[_2] = _3", 3, args, &exception);
    if (exception.ShouldThow()) return Nothing<bool>();
    return _maybe<bool>(true).toMaybe();
}
Maybe<bool> Object::DeletePrivate(Local<Context> context, Local<Private> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (wrap && wrap->m_private_properties) {
        JSValueRef args[] = {
            wrap->m_private_properties,
            V82JSC::ToJSValueRef(key, context)
        };
        LocalException exception(iso);
        V82JSC::exec(ctx, "return delete _1[_2]", 2, args, &exception);
        if (exception.ShouldThow()) return Nothing<bool>();
        return _maybe<bool>(true).toMaybe();
    }
    return _maybe<bool>(false).toMaybe();
}
MaybeLocal<Value> Object::GetPrivate(Local<Context> context, Local<Private> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (wrap && wrap->m_private_properties) {
        JSValueRef args[] = {
            wrap->m_private_properties,
            V82JSC::ToJSValueRef(key, context)
        };
        LocalException exception(iso);
        JSValueRef ret = V82JSC::exec(ctx, "return _1[_2]", 2, args);
        if (exception.ShouldThow()) return MaybeLocal<Value>();
        return ValueImpl::New(V82JSC::ToContextImpl(context), ret);
    }
    return Undefined(context->GetIsolate());
}

/**
 * Returns an array containing the names of the enumerable properties
 * of this object, including properties from prototype objects.  The
 * array returned by this method contains the same values as would
 * be enumerated by a for-in statement over this object.
 */
MaybeLocal<Array> Object::GetPropertyNames(Local<Context> context)
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
    };
    
    JSValueRef ret = V82JSC::exec(ctx->m_ctxRef, "var keys = []; for (var k in _1) keys.push(k); return keys", 1, args, &exception);
    
    if (!exception.ShouldThow()) {
        return ValueImpl::New(ctx, ret).As<Array>();
    }
    return MaybeLocal<Array>();
}
MaybeLocal<Array> Object::GetPropertyNames(Local<Context> context, KeyCollectionMode mode,
                                           PropertyFilter property_filter, IndexFilter index_filter)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    MaybeLocal<Array> array = GetOwnPropertyNames(context, property_filter);
    if (array.IsEmpty()) {
        return MaybeLocal<Array>();
    }
    JSObjectRef arr = (JSObjectRef) V82JSC::ToJSValueRef(array.ToLocalChecked(), context);
    if (mode == KeyCollectionMode::kIncludePrototypes) {
        Local<Value> proto = GetPrototype();
        while (proto->IsObject()) {
            MaybeLocal<Array> proto_properties = proto.As<Object>()->GetOwnPropertyNames(context, property_filter);
            if (proto_properties.IsEmpty()) {
                return MaybeLocal<Array>();
            }
            JSValueRef args[] = {
                arr,
                V82JSC::ToJSValueRef(proto_properties.ToLocalChecked(), context)
            };
            arr = (JSObjectRef) V82JSC::exec(ctx, "return _1.concat(_2)", 2, args);
            proto = proto.As<Object>()->GetPrototype();
        }
    }
    if (index_filter == IndexFilter::kSkipIndices) {
        arr = (JSObjectRef) V82JSC::exec(ctx, "return _1.filter(e=>Number.isNaN(((e)=>{try{return parseInt(e)}catch(x){return parseInt();}})(e)))", 1, &arr);
    }
    return ValueImpl::New(V82JSC::ToContextImpl(context), arr).As<Array>();
}

/**
 * This function has the same functionality as GetPropertyNames but
 * the returned array doesn't contain the names of properties from
 * prototype objects.
 */
MaybeLocal<Array> Object::GetOwnPropertyNames(Local<Context> context)
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef<Object>(this, context),
    };
    
    JSValueRef ret = V82JSC::exec(ctx->m_ctxRef, "return Object.getOwnPropertyNames(_1)", 1, args, &exception);
    
    if (!exception.ShouldThow()) {
        return ValueImpl::New(ctx, ret).As<Array>();
    }
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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(V82JSC::ToIsolateImpl(this));
    
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        JSValueMakeNumber(ctx, filter)
    };
    
    const char *code =
    "const ALL_PROPERTIES = 0;"
    "const ONLY_WRITABLE = 1;"
    "const ONLY_ENUMERABLE = 2;"
    "const ONLY_CONFIGURABLE = 4;"
    "const SKIP_STRINGS = 8;"
    "const SKIP_SYMBOLS = 16;"
    "var filt = _2;"
    "var desc = Object.getOwnPropertyDescriptors(_1);"
    "var arr = [];"
    "for (key in desc) {"
    "    var incl = !filt || filt==SKIP_SYMBOLS ||"
    "    (!(filt&SKIP_STRINGS) &&"
    "     (!(filt&ONLY_WRITABLE)     || desc[key].writable) &&"
    "     (!(filt&ONLY_ENUMERABLE)   || desc[key].enumerable) &&"
    "     (!(filt&ONLY_CONFIGURABLE) || desc[key].configurable));"
    "    if (incl) arr.push(key);"
    "}"
    "var sprops = Object.getOwnPropertySymbols(_1)"
    ".filter(s => {"
    "    var desc = Object.getOwnPropertyDescriptor(_1, s);"
    "    return !filt || filt==SKIP_STRINGS ||"
    "    (!(filt&SKIP_SYMBOLS) &&"
    "     (!(filt&ONLY_WRITABLE)     || desc.writable) &&"
    "     (!(filt&ONLY_ENUMERABLE)   || desc.enumerable) &&"
    "     (!(filt&ONLY_CONFIGURABLE) || desc.configurable));"
    "});"
    "return arr.concat(sprops);";
    
    JSValueRef array = V82JSC::exec(ctx, code, 2, args, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<Array>();
    }
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), array).As<Array>();
}

/**
 * Get the prototype object.  This does not skip objects marked to
 * be skipped by __proto__ and it does not consult the security
 * handler.
 */
Local<Value> Object::GetPrototype()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef obj = V82JSC::ToJSValueRef<Value>(this, context);
    JSValueRef our_proto = V82JSC::GetRealPrototype(context, (JSObjectRef)obj);
    return ValueImpl::New(V82JSC::ToContextImpl(context), our_proto);
}

/**
 * Set the prototype object.  This does not skip objects marked to
 * be skipped by __proto__ and it does not consult the security
 * handler.
 */
Maybe<bool> Object::SetPrototype(Local<Context> context,
                                 Local<Value> prototype)
{
    JSValueRef obj = V82JSC::ToJSValueRef<Value>(this, context);
    Isolate* isolate = V82JSC::ToIsolate(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef new_proto = V82JSC::ToJSValueRef(prototype, isolate);

    bool new_proto_is_hidden = false;
    if (JSValueIsObject(ctx, new_proto)) {
        InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)new_proto);
        new_proto_is_hidden = wrap && wrap->m_isHiddenPrototype;
        if (new_proto_is_hidden) {
            if (JSValueIsStrictEqual(ctx, wrap->m_hidden_proxy_security, new_proto)) {
                // Don't put the hidden proxy in the prototype chain, just the underlying target object
                new_proto = wrap->m_proxy_security ? wrap->m_proxy_security : wrap->m_security;
            }
            // Save a weak reference to this object and propagate our own properties to it
            wrap->m_hidden_children.push_back((JSObjectRef)obj);
            V82JSC::ToImpl<HiddenObjectImpl>(ValueImpl::New(V82JSC::ToContextImpl(context), new_proto))
            ->PropagateOwnPropertiesToChild(context, (JSObjectRef)obj);
        }
    }
    
    V82JSC::SetRealPrototype(context, (JSObjectRef)obj, new_proto);
    
    bool ok = new_proto_is_hidden || GetPrototype()->StrictEquals(prototype);
    if (!ok) return Nothing<bool>();
    return _maybe<bool>(ok).toMaybe();
}

void HiddenObjectImpl::PropagateOwnPropertyToChild(v8::Local<v8::Context> context, v8::Local<v8::Name> property, JSObjectRef child)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        m_value,
        V82JSC::ToJSValueRef(property, context),
        child
    };
    JSValueRef propagate = V82JSC::exec(ctx,
                 "var d = Object.getOwnPropertyDescriptor(_3, _2);"
                 "if (d === undefined) {"
                 "    d = Object.getOwnPropertyDescriptor(_1, _2);"
                 "    if (d === undefined) return false;"
                 "    Object.defineProperty( _3, _2, {"
                 "        get()  { return _1[_2]; },"
                 "        set(v) { return _1[_2] = v; },"
                 "        configurable : d.configurable,"
                 "        enumerable : d.enumerable"
                 "    });"
                 "    return true;"
                 "}"
                 "return false;",
                 3, args);
    if (JSValueToBoolean(ctx, propagate)) {
        InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, child);
        if (wrap && wrap->m_isHiddenPrototype) {
            reinterpret_cast<HiddenObjectImpl*>(V82JSC::ToImpl<ValueImpl>(ValueImpl::New(V82JSC::ToContextImpl(context), child)))
            ->PropagateOwnPropertyToChildren(context, property);
        }
    }
}
void HiddenObjectImpl::PropagateOwnPropertyToChildren(v8::Local<v8::Context> context, v8::Local<v8::Name> property)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)m_value);
    assert(wrap);
    for (auto i=wrap->m_hidden_children.begin(); i != wrap->m_hidden_children.end(); ++i) {
        PropagateOwnPropertyToChild(context, property, *i);
    }
}
void HiddenObjectImpl::PropagateOwnPropertiesToChild(v8::Local<v8::Context> context, JSObjectRef child)
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    HandleScope scope(isolate);
    Local<Object> thiz = V82JSC::CreateLocal<Object>(isolate, this);
    Local<Array> names = thiz->GetOwnPropertyNames(context, ALL_PROPERTIES).ToLocalChecked();
    for (uint32_t i=0; i<names->Length(); i++) {
        Local<Name> property = names->Get(context, i).ToLocalChecked().As<Name>();
        PropagateOwnPropertyToChild(context, property, child);
    }
}

/**
 * Finds an instance of the given function template in the prototype
 * chain.
 */
Local<Object> Object::FindInstanceInPrototypeChain(Local<FunctionTemplate> tmpl)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    Isolate *isolate = V82JSC::ToIsolate(iso);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    FunctionTemplateImpl* tmplimpl = V82JSC::ToImpl<FunctionTemplateImpl>(tmpl);
    
    Local<Value> proto = Local<Value>::New(isolate, this);
    while (proto->IsObject()) {
        JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef(proto, context);
        InstanceWrap *instance_wrap = V82JSC::getPrivateInstance(ctx, obj);
        if (instance_wrap && !instance_wrap->m_object_template.IsEmpty()) {
            Local<ObjectTemplate> objtempl = Local<ObjectTemplate>::New(isolate, instance_wrap->m_object_template);
            Local<FunctionTemplate> t = Local<FunctionTemplate>::New(isolate, V82JSC::ToImpl<ObjectTemplateImpl>(objtempl)->m_constructor_template);
            
            while (!t.IsEmpty()) {
                FunctionTemplateImpl *tt = V82JSC::ToImpl<FunctionTemplateImpl>(t);
                if (tt == tmplimpl) {
                    return proto.As<Object>();
                }
                t = Local<FunctionTemplate>::New(isolate, tt->m_parent);
            }
        }
        proto = proto.As<Object>()->GetPrototype();
    }

    return Local<Object>();
}

/**
 * Call builtin Object.prototype.toString on this object.
 * This is different from Value::ToString() that may call
 * user-defined toString function. This one does not.
 */
MaybeLocal<String> Object::ObjectProtoToString(Local<Context> context)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    LocalException exception(V82JSC::ToIsolateImpl(this));
    JSValueRef s = V82JSC::exec(ctx, "return Object.prototype.toString.call(_1)", 1, &obj, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), s).As<String>();
    }
    
    return MaybeLocal<String>();
}

/**
 * Returns the name of the function invoked as a constructor for this object.
 */
Local<String> Object::GetConstructorName()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSStringRef ctor = JSStringCreateWithUTF8CString("constructor");
    JSValueRef excp = nullptr;
    JSValueRef vctor = JSObjectGetProperty(ctx, (JSObjectRef) obj, ctor, &excp);
    JSStringRelease(ctor);
    assert(excp==nullptr);
    if (JSValueIsObject(ctx, vctor)) {
        JSStringRef name = JSStringCreateWithUTF8CString("name");
        JSValueRef vname = JSObjectGetProperty(ctx, (JSObjectRef) vctor, name, &excp);
        JSStringRelease(name);
        assert(excp==nullptr);
        return ValueImpl::New(V82JSC::ToContextImpl(context), vname)->ToString(context).ToLocalChecked();
    }

    return Local<String>(nullptr);
}

/**
 * Sets the integrity level of the object.
 */
Maybe<bool> Object::SetIntegrityLevel(Local<Context> context, IntegrityLevel level)
{
    assert(0);
    return Nothing<bool>();
}

/** Gets the number of internal fields for this Object. */
int Object::InternalFieldCount()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef(this, context);

    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(this);
    JSContextRef ctx = impl->m_creationCtx;

    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, obj);
    if (wrap) return wrap->m_num_internal_fields;
    else if (IsArrayBufferView()) {
        // ArrayBufferViews have internal fields by default.  This was created in JS.
        GetArrayBufferViewInfo(reinterpret_cast<const ArrayBufferView*>(this));
        return ArrayBufferView::kInternalFieldCount;
    }
    return 0;
}

/** Sets the value in an internal field. */
void Object::SetInternalField(int index, Local<Value> value)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef(this, context);

    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, obj);
    if (!wrap && IsArrayBuffer()) {
        // ArrayBuffers have internal fields by default.  This was created in JS.
        GetArrayBufferInfo(reinterpret_cast<const ArrayBuffer*>(this));
        wrap = V82JSC::getPrivateInstance(ctx, obj);
    }
    if (wrap && index < wrap->m_num_internal_fields) {
        if (wrap->m_internal_fields[index]) {
            JSValueUnprotect(ctx, wrap->m_internal_fields[index]);
        }
        wrap->m_internal_fields[index] = V82JSC::ToJSValueRef(value, context);
        JSValueProtect(ctx, wrap->m_internal_fields[index]);
    }
}

/**
 * Sets a 2-byte-aligned native pointer in an internal field. To retrieve such
 * a field, GetAlignedPointerFromInternalField must be used, everything else
 * leads to undefined behavior.
 */
void Object::SetAlignedPointerInInternalField(int index, void* value)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    SetInternalField(index, External::New(context->GetIsolate(), value));
}
void Object::SetAlignedPointerInInternalFields(int argc, int indices[],
                                       void* values[])
{
    for (int i=0; i<argc; i++) {
        SetAlignedPointerInInternalField(indices[i], values[i]);
    }
}

// Testers for local properties.

/**
 * HasOwnProperty() is like JavaScript's Object.prototype.hasOwnProperty().
 *
 * See also v8::Object::Has() and v8::Object::HasRealNamedProperty().
 */
Maybe<bool> Object::HasOwnProperty(Local<Context> context, Local<Name> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(key, context)
    };
    LocalException exception(iso);
    JSValueRef has = V82JSC::exec(ctx,
                                  "return Object.prototype.hasOwnProperty.call(_1, _2)",
                                  2, args, &exception);
    if (exception.ShouldThow()) {
        return Nothing<bool>();
    }
    
    return _maybe<bool>(JSValueToBoolean(ctx, has)).toMaybe();
}
Maybe<bool> Object::HasOwnProperty(Local<Context> context, uint32_t index)
{
    Local<Value> k = Number::New(V82JSC::ToIsolate(V82JSC::ToContextImpl(context)), index);
    Local<Name> key = k->ToString(context).ToLocalChecked();
    return HasOwnProperty(context, key);
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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);

    JSObjectRef raw_object = (JSObjectRef) V82JSC::ToJSValueRef(this, context);
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, raw_object);
    if (wrap && wrap->m_proxy_security) {
        raw_object = (JSObjectRef) wrap->m_security;
    }

    JSValueRef args[] = {
        raw_object,
        V82JSC::ToJSValueRef(key, context)
    };
    LocalException exception(iso);
    JSValueRef has = V82JSC::exec(ctx,
                                  "return Object.getOwnPropertyDescriptor(_1, _2) !== undefined",
                                  2, args, &exception);
    if (exception.ShouldThow()) {
        return Nothing<bool>();
    }
    
    return _maybe<bool>(JSValueToBoolean(ctx, has)).toMaybe();
}
Maybe<bool> Object::HasRealIndexedProperty(Local<Context> context, uint32_t index)
{
    return HasRealNamedProperty(context, Uint32::New(context->GetIsolate(), index).As<Name>());
}
Maybe<bool> Object::HasRealNamedCallbackProperty(Local<Context> context, Local<Name> key)
{
    assert(0);
    return Nothing<bool>();
}

/**
 * If result.IsEmpty() no real property was located in the prototype chain.
 * This means interceptors in the prototype chain are not called.
 */
MaybeLocal<Value> Object::GetRealNamedPropertyInPrototypeChain(Local<Context> context,
                                                               Local<Name> key)
{
    Local<Value> proto = GetPrototype();
    return proto.As<Object>()->GetRealNamedProperty(context, key);
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
    Local<Value> proto = GetPrototype();
    return proto.As<Object>()->GetRealNamedPropertyAttributes(context, key);
}

/**
 * If result.IsEmpty() no real property was located on the object or
 * in the prototype chain.
 * This means interceptors in the prototype chain are not called.
 */
MaybeLocal<Value> Object::GetRealNamedProperty(Local<Context> context, Local<Name> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(V82JSC::ToIsolateImpl(this));
    
    JSObjectRef raw_object = (JSObjectRef) V82JSC::ToJSValueRef(this, context);
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, raw_object);
    if (wrap && wrap->m_proxy_security) {
        raw_object = (JSObjectRef) wrap->m_security;
    }

    JSValueRef args[] = {
        raw_object,
        V82JSC::ToJSValueRef(key, context)
    };
    
    const char *code =
    "function getreal(o,p) { "
    "    var d = Object.getOwnPropertyDescriptor(o,p);"
    "    return typeof(d) === 'undefined' && o.__proto__ ? "
    "        getreal(o.__proto__,p) : "
    "        typeof(d) === 'undefined' ? (function(){ throw 0; })() :"
    "        'value' in d ? d.value :"
    "        (function(){ throw new Error(); })(); "
    "}"
    "return getreal(_1,_2);";
    
    JSValueRef value = V82JSC::exec(ctx, code, 2, args, &exception);
    if (exception.ShouldThow()) {
        if (JSValueIsStrictEqual(ctx, exception.exception_, JSValueMakeNumber(ctx, 0))) {
            exception.Clear();
        }
        return MaybeLocal<Value>();
    }
    
    return ValueImpl::New(V82JSC::ToContextImpl(context), value);
}

/**
 * Gets the property attributes of a real property which can be
 * None or any combination of ReadOnly, DontEnum and DontDelete.
 * Interceptors in the prototype chain are not called.
 */
Maybe<PropertyAttribute> Object::GetRealNamedPropertyAttributes(Local<Context> context, Local<Name> key)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    TryCatch try_catch(V82JSC::ToIsolate(this));

    PropertyAttribute retval = PropertyAttribute::None;
    {
        LocalException exception(V82JSC::ToIsolateImpl(this));
        
        JSObjectRef raw_object = (JSObjectRef) V82JSC::ToJSValueRef(this, context);
        InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, raw_object);
        if (wrap && wrap->m_proxy_security) {
            raw_object = (JSObjectRef) wrap->m_security;
        }

        JSValueRef args[] = {
            raw_object,
            V82JSC::ToJSValueRef(key, context)
        };
        
        const char *code =
        "const None = 0;"
        "const ReadOnly = 1 << 0;"
        "const DontEnum = 1 << 1;"
        "const DontDelete = 1 << 2;"
        "function getreal(o,p) { "
        "    var d = Object.getOwnPropertyDescriptor(o,p);"
        "    return typeof(d) === 'undefined' && o.__proto__ ? "
        "        getreal(o.__proto__,p) : 'value' in d ? d : (function(){ throw new Error(); })(); "
        "}"
        "var desc = getreal(_1,_2); "
        "return 0 + (desc.writable ? 0 : ReadOnly) + (desc.enumerable ? 0 : DontEnum) + (desc.configurable ? 0 : DontDelete);";

        JSValueRef value = V82JSC::exec(ctx, code, 2, args, &exception);
        if (!exception.ShouldThow()) {
            retval = (PropertyAttribute)JSValueToNumber(ctx, value, 0);
        }
    }
    
    return _maybe<PropertyAttribute>(retval).toMaybe();
}

/** Tests for a named lookup interceptor.*/
bool Object::HasNamedLookupInterceptor()
{
    assert(0);
    return false;
}

/** Tests for an index lookup interceptor.*/
bool Object::HasIndexedLookupInterceptor()
{
    assert(0);
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
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (!wrap) {
        wrap = V82JSC::makePrivateInstance(V82JSC::ToIsolateImpl(this), ctx, (JSObjectRef)obj);
    }
    return wrap->m_hash;
}

/**
 * Clone this object with a fast but shallow copy.  Values will point
 * to the same values as the original object.
 */
// TODO(dcarney): take an isolate and optionally bail out?
Local<Object> Object::Clone()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSValueRef newobj = V82JSC::exec(ctx, "return {..._1}", 1, &obj);
    return ValueImpl::New(V82JSC::ToContextImpl(context), newobj).As<Object>();
}

/**
 * Returns the context in which the object was created.
 */
Local<Context> Object::CreationContext()
{
    assert(0);
    return Local<Context>();
}

/**
 * Checks whether a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 * When an Object is callable this method returns true.
 */
bool Object::IsCallable()
{
    assert(0);
    return false;
}

/**
 * True if this object is a constructor.
 */
bool Object::IsConstructor()
{
    assert(0);
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
    assert(0);
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
    assert(0);
    return MaybeLocal<Value>();
}

Local<Object> Object::New(Isolate* isolate)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    Local<Value> o = ValueImpl::New(V82JSC::ToContextImpl(context), JSObjectMake(ctx, 0, 0));
    return o.As<Object>();
}

void* Object::SlowGetAlignedPointerFromInternalField(int index)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);

    Local<External> external = SlowGetInternalField(index).As<External>();
    JSObjectRef o = (JSObjectRef) V82JSC::ToJSValueRef(external, context);
    return JSObjectGetPrivate(o);
}

Local<Value> Object::SlowGetInternalField(int index)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef(this, context);
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, obj);
    if (wrap && index < wrap->m_num_internal_fields) {
        if (wrap->m_internal_fields[index]) {
            return ValueImpl::New(V82JSC::ToContextImpl(context), wrap->m_internal_fields[index]);
        } else {
            return Undefined(context->GetIsolate());
        }
    }
    return Local<Value>();
}

