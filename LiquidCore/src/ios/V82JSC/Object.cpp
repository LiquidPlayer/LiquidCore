/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "ObjectTemplate.h"
#include "FunctionTemplate.h"
#include "Object.h"

using namespace V82JSC;
using v8::Local;
using v8::EscapableHandleScope;
using v8::HandleScope;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Isolate;
using v8::Object;
using v8::TryCatch;

Maybe<bool> Object::Set(Local<Context> context, Local<Value> key, Local<Value> value)
{
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);

    Local<v8::Value> thiz = CreateLocal<v8::Value>(&iso->ii, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    value = V82JSC::TrackedObject::SecureValue(value, thiz.As<v8::Object>()->CreationContext());
    
    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context),
        ToJSValueRef(value, context)
    };
    
    JSValueRef ret = exec(ctx, "return _3 == (_1[_2] = _3)", 3, args, &exception);
    
    _maybe<bool> out;
    if (!exception.ShouldThrow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThrow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Set(Local<Context> context, uint32_t index,
                                      Local<Value> value)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<v8::Value> thiz = CreateLocal<v8::Value>(&iso->ii, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    value = V82JSC::TrackedObject::SecureValue(value);

    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(thiz, context);
    JSValueRef value_ = ToJSValueRef(value, context);

    JSValueRef exception = nullptr;
    JSObjectSetPropertyAtIndex(ctx, (JSObjectRef)obj, index, value_, &exception);
    
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
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> k = Number::New(ToIsolate(ToContextImpl(context)), index);
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
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    value = V82JSC::TrackedObject::SecureValue(value);
    
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(thiz, context);
    JSValueRef key_ = ToJSValueRef(key, context);
    JSValueRef v = ToJSValueRef(value, context);

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
    bool success = true;
    LocalException exception(ToIsolateImpl(this));
    exec(ctx,
                 "Object.defineProperty(_1, _2, "
                 "{ writable : !(_4&(1<<0)), "
                 "  enumerable : !(_4&(1<<1)), "
                 "  configurable : !(_4&(1<<2)), "
                 "  value: _3 })", 4, args, &exception);
    if (exception.ShouldThrow()) {
        success = false;
        JSStringRef err = JSValueToStringCopy(ctx, exception.exception_, 0);
        char e[JSStringGetMaximumUTF8CStringSize(err)];
        JSStringGetUTF8CString(err, e, JSStringGetMaximumUTF8CStringSize(err));
        if (strstr(e, "access denied")) {
            return Nothing<bool>();
        }
        exception.exception_ = 0;
    }

    return _maybe<bool>(success).toMaybe();
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
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    static JSStringRef
    get_ = JSStringCreateWithUTF8CString("get"),
    set_ = JSStringCreateWithUTF8CString("set"),
    value_ = JSStringCreateWithUTF8CString("value"),
    writable_ = JSStringCreateWithUTF8CString("writable"),
    enumerable_ = JSStringCreateWithUTF8CString("enumerable"),
    configurable_ = JSStringCreateWithUTF8CString("configurable");
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(thiz, context);
    JSValueRef key_ = ToJSValueRef(key, context);
    
    JSObjectRef desc = JSObjectMake(ctx, 0, 0);
    if (descriptor.has_get()) {
        JSObjectSetProperty(ctx, desc, get_, ToJSValueRef(descriptor.get(), context), 0, 0);
    }
    if (descriptor.has_set()) {
        JSObjectSetProperty(ctx, desc, set_, ToJSValueRef(descriptor.set(), context), 0, 0);
    }
    if (descriptor.has_writable()) {
        JSObjectSetProperty(ctx, desc, writable_, JSValueMakeBoolean(ctx, descriptor.writable()), 0, 0);
    }
    if (descriptor.has_enumerable()) {
        JSObjectSetProperty(ctx, desc, enumerable_, JSValueMakeBoolean(ctx, descriptor.enumerable()), 0, 0);
    }
    if (descriptor.has_configurable()) {
        JSObjectSetProperty(ctx, desc, configurable_, JSValueMakeBoolean(ctx, descriptor.configurable()), 0, 0);
    }
    if (descriptor.has_value()) {
        JSObjectSetProperty(ctx, desc, value_, ToJSValueRef(descriptor.value(), context), 0, 0);
    }

    TryCatch try_catch(ToIsolate(this));
    {
        LocalException exception(ToIsolateImpl(this));
        JSValueRef args[] = {
            obj,
            key_,
            desc
        };
        exec(ctx, "Object.defineProperty(_1, _2, _3);", 3, args, &exception);
    }
    if (try_catch.HasCaught()) {
        JSValueRef exception = ToJSValueRef(try_catch.Exception(), context);
        JSStringRef err = JSValueToStringCopy(ctx, exception, 0);
        char e[JSStringGetMaximumUTF8CStringSize(err)];
        JSStringGetUTF8CString(err, e, JSStringGetMaximumUTF8CStringSize(err));
        if (strstr(e, "access denied")) {
            JSStringRelease(err);
            try_catch.ReThrow();
            return Nothing<bool>();
        }
        JSStringRelease(err);
        return _maybe<bool>(false).toMaybe();
    }

    return _maybe<bool>(true).toMaybe();
}

// Sets an own property on this object bypassing interceptors and
// overriding accessors or read-only properties.
//
// Note that if the object has an interceptor the property will be set
// locally, but since the interceptor takes precedence the local property
// will only be returned if the interceptor doesn't return a value.
//
// Note also that this only works for named properties.
MaybeLocal<v8::Value> Object::Get(Local<Context> context, Local<Value> key)
{
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    EscapableHandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<v8::Value>(&iso->ii, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context)
    };
    
    JSValueRef ret = exec(ctx, "return Reflect.get(_1,_2)", 2, args, &exception);
    
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), ret));
    }
    return MaybeLocal<Value>();
}

MaybeLocal<v8::Value> Object::Get(Local<Context> context, uint32_t index)
{
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    EscapableHandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSValueRef obj = ToJSValueRef(thiz, context);

    LocalException exception(iso);
    JSValueRef prop = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)obj, index, &exception);
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), prop));
    }
    
    return MaybeLocal<Value>();
}

/**
 * Gets the property attributes of a property which can be None or
 * any combination of ReadOnly, DontEnum and DontDelete. Returns
 * None when the property doesn't exist.
 */
Maybe<v8::PropertyAttribute> Object::GetPropertyAttributes(Local<Context> context, Local<Value> key)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context),
    };
    JSValueRef ret = exec(ctx,
                                  "const None = 0, ReadOnly = 1 << 0, DontEnum = 1 << 1, DontDelete = 1 << 2; "
                                  "var d = Object.getOwnPropertyDescriptor(_1, _2); "
                                  "var attr = None; if (!d) return attr; "
                                  "attr += (d.writable===true) ? 0 : ReadOnly; "
                                  "attr += (d.enumerable===true) ? 0 : DontEnum; "
                                  "attr += (d.configurable===true) ? 0 : DontDelete; "
                                  "return attr"
                                  , 2, args, &exception);

    _maybe<PropertyAttribute> out;
    if (!exception.ShouldThrow()) {
        JSValueRef excp = 0;
        out.value_ = (PropertyAttribute) JSValueToNumber(ctx, ret, &excp);
        assert(excp==0);
    }
    out.has_value_ = !exception.ShouldThrow();
    
    return out.toMaybe();
}

/**
 * Returns Object.getOwnPropertyDescriptor as per ES2016 section 19.1.2.6.
 */
MaybeLocal<v8::Value> Object::GetOwnPropertyDescriptor(Local<Context> context, Local<Name> key)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context)
    };
    LocalException exception(iso);
    JSValueRef descriptor = exec(ctx,
                                         "return Object.getOwnPropertyDescriptor(_1, _2)",
                                         2, args, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<Value>();
    }
    
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), descriptor));
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
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
        (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this)));

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context)
    };
    
    JSValueRef ret = exec(ctx, "return (_2 in _1)", 2, args, &exception);
    
    _maybe<bool> out;
    if (!exception.ShouldThrow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThrow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Delete(Local<Context> context, Local<Value> key)
{
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
        (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this)));

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context)
    };

    JSValueRef ret = exec(ctx, "return delete _1[_2]", 2, args, &exception);

    _maybe<bool> out;
    if (!exception.ShouldThrow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThrow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Has(Local<Context> context, uint32_t index)
{
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
    (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this)));

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        JSValueMakeNumber(ctx, index)
    };
    JSValueRef ret = exec(ctx, "return (_2 in _1)", 2, args, &exception);
    
    _maybe<bool> out;
    if (!exception.ShouldThrow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThrow();
    
    return out.toMaybe();
}

Maybe<bool> Object::Delete(Local<Context> context, uint32_t index)
{
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
    (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this)));
    
    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        JSValueMakeNumber(ctx, index)
    };
    
    JSValueRef ret = exec(ctx, "return Reflect.deleteProperty(_1, _2)", 2, args, &exception);

    _maybe<bool> out;
    if (!exception.ShouldThrow()) {
        out.value_ = JSValueToBoolean(ctx, ret);
    }
    out.has_value_ = !exception.ShouldThrow();
    
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
                                PropertyAttribute attribute,
                                SideEffectType getter_side_effect_type)
{
    ObjectImpl* thiz = static_cast<ObjectImpl*>(this);
    return thiz->SetAccessor(context, name, getter, setter, data,
                             settings, attribute, Local<Signature>());
}

Maybe<bool> ObjectImpl::SetAccessor(Local<v8::Context> context,
                                            Local<v8::Name> name,
                                            v8::AccessorNameGetterCallback getter,
                                            v8::AccessorNameSetterCallback setter,
                                            MaybeLocal<v8::Value> data,
                                            v8::AccessControl settings,
                                            v8::PropertyAttribute attribute,
                                            Local<v8::Signature> signature)
{
    auto ctximpl = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    v8::Context::Scope context_scope(context);
    
    Local<Object> thiz = TrackedObject::SecureValue
    (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this))).As<Object>();

    const auto callback = [](JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                             size_t argumentCount, const JSValueRef *arguments, JSValueRef *exception) -> JSValueRef
    {
        IsolateImpl *iso = IsolateFromCtx(ctx);
        Isolate* isolate = ToIsolate(iso);
        v8::Locker lock(isolate);

        HandleScope scope (isolate);
        void * persistent = JSObjectGetPrivate(function);
        Local<v8::AccessorInfo> acc_info = FromPersistentData<v8::AccessorInfo>(ToIsolate(iso), persistent);
        auto wrap = ToImpl<Accessor>(acc_info);
        auto thread = IsolateImpl::PerThreadData::Get(iso);
        
        Local<v8::Context> context = LocalContext::New(isolate, ctx);
        v8::Context::Scope context_scope(context);
        auto ctximpl = ToContextImpl(context);
        
        Local<Value> thiz = V82JSC::Value::New(ctximpl, thisObject);
        Local<Value> data = V82JSC::Value::New(ctximpl, wrap->m_data);
        Local<Value> holder = V82JSC::Value::New(ctximpl, wrap->m_holder);

        // Check signature
        bool signature_match = wrap->signature.IsEmpty();
        if (!signature_match) {
            auto sig = ToImpl<Signature>(wrap->signature.Get(isolate));
            auto sig_templ = ToImpl<Template>(sig->m_template.Get(isolate));
            JSValueRef proto = thisObject;
            auto thisWrap = TrackedObject::getPrivateInstance(ctx, (JSObjectRef)proto);
            while(!signature_match && JSValueIsObject(ctx, proto)) {
                if (thisWrap && !thisWrap->m_object_template.IsEmpty() && (proto == thisObject || thisWrap->m_isHiddenPrototype)) {
                    holder = proto == thisObject? thiz : V82JSC::Value::New(ctximpl, proto);
                    auto ot = ToImpl<ObjectTemplate>(thisWrap->m_object_template.Get(isolate));
                    Local<v8::FunctionTemplate> ctort = ot->m_constructor_template.Get(isolate);
                    auto templ = ctort.IsEmpty() ? nullptr : ToImpl<Template>(ctort);
                    while (!signature_match && templ) {
                        signature_match = sig_templ == templ;
                        templ = templ->m_parent.IsEmpty() ? nullptr : ToImpl<Template>(templ->m_parent.Get(isolate));
                    }
                }
                proto = GetRealPrototype(context, (JSObjectRef)proto);
                thisWrap = TrackedObject::getPrivateInstance(ctx, (JSObjectRef)proto);
                if (!thisWrap || !thisWrap->m_isHiddenPrototype) break;
            }
        }
        if (!signature_match) {
            JSStringRef message = JSStringCreateWithUTF8CString("new TypeError('Illegal invocation')");
            *exception = JSEvaluateScript(ctx, message, 0, 0, 0, 0);
            JSStringRelease(message);
            return 0;
        }

        typedef v8::internal::Heap::RootListIndex R;
        v8::internal::Object *the_hole = iso->ii.heap()->root(R::kTheHoleValueRootIndex);

        // FIXME: I can think of no way to determine whether we were called from strict mode or not
        bool isStrict = false;
        v8::internal::Object *shouldThrow = v8::internal::Smi::FromInt(isStrict?1:0);
        
        thread->m_callback_depth ++;
        
        v8::internal::Object * implicit[] = {
            shouldThrow,                                             // kShouldThrowOnErrorIndex = 0;
            * reinterpret_cast<v8::internal::Object**>(*holder),     // kHolderIndex = 1;
            O(iso),                                                  // kIsolateIndex = 2;
            the_hole,                                                // kReturnValueDefaultValueIndex = 3;
            the_hole,                                                // kReturnValueIndex = 4;
            * reinterpret_cast<v8::internal::Object**>(*data),       // kDataIndex = 5;
            * reinterpret_cast<v8::internal::Object**>(*thiz),       // kThisIndex = 6;
        };
        
        thread->m_scheduled_exception = the_hole;
        TryCatch try_catch(ToIsolate(iso));

        Local<v8::Value> ret = Undefined(ToIsolate(iso));
        if (argumentCount == 0) {
            PropertyCallback<v8::Value> info(implicit);
            wrap->getter(V82JSC::Value::New(ctximpl, wrap->m_property).As<v8::Name>(), info);
            ret = info.GetReturnValue().Get();
        } else {
            PropertyCallback<void> info(implicit);
            {
                v8::Unlocker unlock(isolate);
                wrap->setter(V82JSC::Value::New(ctximpl, wrap->m_property).As<v8::Name>(),
                             V82JSC::Value::New(ctximpl, arguments[0]),
                             info);
            }
        }
        
        if (try_catch.HasCaught()) {
            *exception = ToJSValueRef(try_catch.Exception(), context);
        } else if (thread->m_scheduled_exception != the_hole) {
            v8::internal::Object * excep = thread->m_scheduled_exception;
            *exception = ToJSValueRef_<Value>(excep, context);
            thread->m_scheduled_exception = the_hole;
        }

        -- thread->m_callback_depth;

        return ToJSValueRef<Value>(ret, context);
    };
    
    auto wrap = static_cast<Accessor*>(HeapAllocator::Alloc(iso, iso->m_accessor_map));
    
    wrap->m_property = ToJSValueRef(name, context);
    JSValueProtect(ctximpl->m_ctxRef, wrap->m_property);
    wrap->getter = getter;
    wrap->setter = setter;
    if (data.IsEmpty()) data = Undefined(ToIsolate(iso));
    wrap->m_data = ToJSValueRef(data.ToLocalChecked(), context);
    JSValueProtect(ctximpl->m_ctxRef, wrap->m_data);
    wrap->m_holder = ToJSValueRef(thiz, context);
    wrap->signature.Reset(ToIsolate(iso), signature);
    
    void *persistent = PersistentData(ToIsolate(iso), CreateLocal<v8::AccessorInfo>(&iso->ii, wrap));
    
    JSClassDefinition def = kJSClassDefinitionEmpty;
    def.attributes = kJSClassAttributeNoAutomaticPrototype;
    def.callAsFunction = callback;
    def.finalize = [](JSObjectRef obj)
    {
        void * data = JSObjectGetPrivate(obj);
        ReleasePersistentData<v8::AccessorInfo>(data);
    };
    JSClassRef claz = JSClassCreate(&def);
    JSObjectRef accessor_function = JSObjectMake(ctximpl->m_ctxRef, claz, persistent);
    JSClassRelease(claz);
    Local<v8::Function> accessor = V82JSC::Value::New(ctximpl, accessor_function).As<v8::Function>();
    
    TryCatch try_catch(ToIsolate(iso));
    
    thiz->SetAccessorProperty(name,
                              (getter) ? accessor : Local<v8::Function>(),
                              (setter) ? accessor : Local<v8::Function>(),
                              attribute,
                              settings);
    bool success = true;
    if (try_catch.HasCaught()) {
        success = false;
        JSValueRef exception = ToJSValueRef(try_catch.Exception(), context);
        JSStringRef err = JSValueToStringCopy(ctximpl->m_ctxRef, exception, 0);
        char e[JSStringGetMaximumUTF8CStringSize(err)];
        JSStringGetUTF8CString(err, e, JSStringGetMaximumUTF8CStringSize(err));
        if (strstr(e, "access denied")) {
            JSStringRelease(err);
            try_catch.ReThrow();
            return v8::Nothing<bool>();
        }
    }
    
    return _maybe<bool>(success).toMaybe();
}

void Object::SetAccessorProperty(Local<Name> name, Local<Function> getter,
                                 Local<Function> setter,
                                 PropertyAttribute attribute,
                                 AccessControl settings)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    
    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
        (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this)));
    
    Local<Context> context = ToCurrentContext(this);

    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(thiz, context);
    JSValueRef prop = ToJSValueRef(name, context);

    LocalException exception(iso);
    
    if (settings != AccessControl::DEFAULT) {
        auto wrap = V82JSC::TrackedObject::makePrivateInstance(iso, ctx, obj);
        if (!wrap->m_access_control) {
            wrap->m_access_control = JSObjectMake(ctx, 0, 0);
            JSValueProtect(ctx, wrap->m_access_control);
        }
        JSValueRef args[] = {
            wrap->m_access_control,
            prop,
            JSValueMakeNumber(ctx, settings)
        };
        exec(ctx, "_1[_2] = _3", 3, args);
    }

    JSValueRef args[] = {
        obj,
        prop,
        !getter.IsEmpty() ? ToJSValueRef(getter, context) : 0,
        !setter.IsEmpty() ? ToJSValueRef(setter, context) : 0,
        JSValueMakeNumber(ctx, attribute)
    };
    
    /* None = 0,
     ReadOnly = 1 << 0, // Not used with accessors
     DontEnum = 1 << 1,
     DontDelete = 1 << 2
     */
    exec(ctx,
                 "delete _1[_2]; "
                 "var desc = "
                 "{ "
                 "  enumerable : !(_5&(1<<1)), "
                 "  configurable : !(_5&(1<<2))}; "
                 "if (_4===null) { desc.get = _3; desc.set = function(v) { delete this[_2]; this[_2] = v; }; }"
                 "else if (_3===null) { desc.set = _4; }"
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
                                          Local<Value> data, PropertyAttribute attributes,
                                          SideEffectType getter_side_effect_type)
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
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (wrap && wrap->m_private_properties) {
        JSValueRef args[] = {
            wrap->m_private_properties,
            ToJSValueRef(key, context)
        };
        LocalException exception(iso);
        JSValueRef ret = exec(ctx, "return _1.hasOwnProperty(_2)", 2, args);
        if (exception.ShouldThrow()) return Nothing<bool>();
        return _maybe<bool>(JSValueToBoolean(ctx, ret)).toMaybe();
    }
    return _maybe<bool>(false).toMaybe();
}
Maybe<bool> Object::SetPrivate(Local<Context> context, Local<Private> key,
                               Local<Value> value)
{
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (!wrap) wrap = V82JSC::TrackedObject::makePrivateInstance(iso, ctx, (JSObjectRef)obj);
    if (!wrap->m_private_properties) {
        wrap->m_private_properties = JSObjectMake(ctx, 0, 0);
        JSValueProtect(ctx, wrap->m_private_properties);
    }
    JSValueRef args[] = {
        wrap->m_private_properties,
        ToJSValueRef(key, context),
        ToJSValueRef(value, context)
    };
    LocalException exception(iso);
    exec(ctx, "_1[_2] = _3", 3, args, &exception);
    if (exception.ShouldThrow()) return Nothing<bool>();
    return _maybe<bool>(true).toMaybe();
}
Maybe<bool> Object::DeletePrivate(Local<Context> context, Local<Private> key)
{
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (wrap && wrap->m_private_properties) {
        JSValueRef args[] = {
            wrap->m_private_properties,
            ToJSValueRef(key, context)
        };
        LocalException exception(iso);
        exec(ctx, "return delete _1[_2]", 2, args, &exception);
        if (exception.ShouldThrow()) return Nothing<bool>();
        return _maybe<bool>(true).toMaybe();
    }
    return _maybe<bool>(false).toMaybe();
}
MaybeLocal<v8::Value> Object::GetPrivate(Local<Context> context, Local<Private> key)
{
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    IsolateImpl* iso = ToIsolateImpl(this);
    EscapableHandleScope scope(ToIsolate(iso));

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (wrap && wrap->m_private_properties) {
        JSValueRef args[] = {
            wrap->m_private_properties,
            ToJSValueRef(key, context)
        };
        LocalException exception(iso);
        JSValueRef ret = exec(ctx, "return _1[_2]", 2, args);
        if (exception.ShouldThrow()) return MaybeLocal<Value>();
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), ret));
    }
    return scope.Escape(Undefined(context->GetIsolate()));
}

/**
 * Returns an array containing the names of the enumerable properties
 * of this object, including properties from prototype objects.  The
 * array returned by this method contains the same values as would
 * be enumerated by a for-in statement over this object.
 */
MaybeLocal<v8::Array> Object::GetPropertyNames(Local<Context> context)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
    };
    
    JSValueRef ret = exec(ctx->m_ctxRef, "var keys = []; for (var k in _1) keys.push(k); return keys", 1, args, &exception);
    
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ctx, ret).As<Array>());
    }
    return MaybeLocal<Array>();
}
MaybeLocal<v8::Array> Object::GetPropertyNames(Local<v8::Context> context, KeyCollectionMode mode,
                                               PropertyFilter property_filter, IndexFilter index_filter,
                                               KeyConversionMode key_conversion)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Object> thiz = CreateLocal<Object>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz).As<Object>();
    JSContextRef ctx = ToContextRef(context);
    MaybeLocal<Array> array = thiz->GetOwnPropertyNames(context, property_filter);
    if (array.IsEmpty()) {
        return MaybeLocal<Array>();
    }
    JSObjectRef arr = (JSObjectRef) ToJSValueRef(array.ToLocalChecked(), context);
    if (mode == KeyCollectionMode::kIncludePrototypes) {
        Local<Value> proto = GetPrototype();
        while (proto->IsObject()) {
            MaybeLocal<Array> proto_properties = proto.As<Object>()->GetOwnPropertyNames(context, property_filter);
            if (proto_properties.IsEmpty()) {
                return MaybeLocal<Array>();
            }
            JSValueRef args[] = {
                arr,
                ToJSValueRef(proto_properties.ToLocalChecked(), context)
            };
            arr = (JSObjectRef) exec(ctx, "return _1.concat(_2)", 2, args);
            proto = proto.As<Object>()->GetPrototype();
        }
    }
    if (index_filter == IndexFilter::kSkipIndices) {
        arr = (JSObjectRef) exec(ctx, "return _1.filter(e=>Number.isNaN(((e)=>{try{return parseInt(e)}catch(x){return parseInt();}})(e)))", 1, &arr);
    }
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), arr).As<Array>());
}

/**
 * This function has the same functionality as GetPropertyNames but
 * the returned array doesn't contain the names of properties from
 * prototype objects.
 */
MaybeLocal<v8::Array> Object::GetOwnPropertyNames(Local<Context> context)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(this);

    LocalException exception(iso);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
    };
    
    JSValueRef ret = exec(ctx->m_ctxRef, "return Object.getOwnPropertyNames(_1)", 1, args, &exception);
    
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ctx, ret).As<Array>());
    }
    return MaybeLocal<Array>();
}

/**
 * Returns an array containing the names of the filtered properties
 * of this object, including properties from prototype objects.  The
 * array returned by this method contains the same values as would
 * be enumerated by a for-in statement over this object.
 */
MaybeLocal<v8::Array> Object::GetOwnPropertyNames(Local<Context> context, PropertyFilter filter,
                                                  KeyConversionMode key_conversion)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    LocalException exception(ToIsolateImpl(this));
    
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
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
    
    JSValueRef array = exec(ctx, code, 2, args, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<Array>();
    }
    
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), array).As<Array>());
}

/**
 * Get the prototype object.  This does not skip objects marked to
 * be skipped by __proto__ and it does not consult the security
 * handler.
 */
Local<v8::Value> Object::GetPrototype()
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = ToCurrentContext(this);
    JSValueRef obj = ToJSValueRef<Value>(this, context);
    JSValueRef our_proto = GetRealPrototype(context, (JSObjectRef)obj);
    
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), our_proto));
}

/**
 * Set the prototype object.  This does not skip objects marked to
 * be skipped by __proto__ and it does not consult the security
 * handler.
 */
Maybe<bool> Object::SetPrototype(Local<Context> context,
                                 Local<Value> prototype)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    JSValueRef obj = ToJSValueRef(thiz, context);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef new_proto = ToJSValueRef(prototype, context);

    bool new_proto_is_hidden = false;
    if (JSValueIsObject(ctx, new_proto)) {
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)new_proto);
        new_proto_is_hidden = wrap && wrap->m_isHiddenPrototype;
        if (new_proto_is_hidden) {
            /* FIXME: This doesn't seem to be necessary.  Remove?
            if (JSValueIsStrictEqual(ctx, wrap->m_hidden_proxy_security, new_proto)) {
                // Don't put the hidden proxy in the prototype chain, just the underlying target object
                new_proto = wrap->m_proxy_security ? wrap->m_proxy_security : wrap->m_security;
            }
            */
            // Save a reference to this object and propagate our own properties to it
            if (!wrap->m_hidden_children_array) {
                wrap->m_hidden_children_array = JSObjectMakeArray(ctx, 0, nullptr, 0);
                JSValueProtect(ctx, wrap->m_hidden_children_array);
            }
            JSValueRef args[] = { wrap->m_hidden_children_array, obj };
            exec(ctx, "_1.push(_2)", 2, args);
            ToImpl<HiddenObject>(V82JSC::Value::New(ToContextImpl(context), new_proto))
                ->PropagateOwnPropertiesToChild(context, (JSObjectRef)obj);
        }
    }
    
    TryCatch try_catch(isolate);
    
    SetRealPrototype(context, (JSObjectRef)obj, new_proto);
    
    bool ok = new_proto_is_hidden || GetPrototype()->StrictEquals(prototype);
    if (!ok) return Nothing<bool>();
    return _maybe<bool>(ok).toMaybe();
}

void HiddenObject::PropagateOwnPropertyToChild(v8::Local<v8::Context> context, v8::Local<v8::Name> property, JSObjectRef child)
{
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        m_value,
        ToJSValueRef(property, context),
        child
    };
    JSValueRef propagate = exec(ctx,
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
        auto wrap = TrackedObject::getPrivateInstance(ctx, child);
        if (wrap && wrap->m_isHiddenPrototype) {
            reinterpret_cast<HiddenObject*>(ToImpl<Value>(Value::New(ToContextImpl(context), child)))
            ->PropagateOwnPropertyToChildren(context, property);
        }
    }
}
void HiddenObject::PropagateOwnPropertyToChildren(v8::Local<v8::Context> context, v8::Local<v8::Name> property)
{
    JSContextRef ctx = ToContextRef(context);
    auto wrap = TrackedObject::getPrivateInstance(ctx, (JSObjectRef)m_value);
    assert(wrap);
    
    if (wrap->m_hidden_children_array) {
        int length = static_cast<int>(JSValueToNumber(ctx, exec(ctx, "return _1.length",
                                                                        1, &wrap->m_hidden_children_array), 0));
        for (unsigned i=0; i < length; ++i) {
            JSValueRef child = JSObjectGetPropertyAtIndex(ctx, wrap->m_hidden_children_array, i, 0);
            assert(JSValueIsObject(ctx, child));
            PropagateOwnPropertyToChild(context, property, (JSObjectRef)child);
        }
    }
}
void HiddenObject::PropagateOwnPropertiesToChild(v8::Local<v8::Context> context, JSObjectRef child)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Object> thiz = CreateLocal<Object>(isolate, this);
    Local<v8::Array> names = thiz->GetOwnPropertyNames(context, v8::ALL_PROPERTIES).ToLocalChecked();
    for (uint32_t i=0; i<names->Length(); i++) {
        Local<v8::Name> property = names->Get(context, i).ToLocalChecked().As<v8::Name>();
        PropagateOwnPropertyToChild(context, property, child);
    }
}

/**
 * Finds an instance of the given function template in the prototype
 * chain.
 */
Local<Object> Object::FindInstanceInPrototypeChain(Local<FunctionTemplate> tmpl)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    Isolate *isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);

    auto tmplimpl = ToImpl<V82JSC::FunctionTemplate>(tmpl);
    
    Local<Value> proto = Local<Value>::New(isolate, this);
    while (proto->IsObject()) {
        JSObjectRef obj = (JSObjectRef) ToJSValueRef(proto, context);
        auto instance_wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
        if (instance_wrap && !instance_wrap->m_object_template.IsEmpty()) {
            Local<ObjectTemplate> objtempl = Local<ObjectTemplate>::New(isolate, instance_wrap->m_object_template);
            Local<FunctionTemplate> t = Local<FunctionTemplate>::New(isolate, ToImpl<V82JSC::ObjectTemplate>(objtempl)->m_constructor_template);
            
            while (!t.IsEmpty()) {
                auto tt = ToImpl<V82JSC::FunctionTemplate>(t);
                if (tt == tmplimpl) {
                    return scope.Escape(proto.As<Object>());
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
MaybeLocal<v8::String> Object::ObjectProtoToString(Local<Context> context)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSGlobalContextRef ctx = JSContextGetGlobalContext(ToContextRef(context));
    Local<Context> global_context = ToIsolateImpl(this)->m_global_contexts[ctx].Get(ToIsolate(this));
    Context::Scope context_scope(context);
    
    JSObjectRef toString = (JSObjectRef)
        ToJSValueRef(ToGlobalContextImpl(global_context)
                             ->ObjectPrototypeToString.Get(ToIsolate(this)), context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(thiz, context);

    LocalException exception(ToIsolateImpl(this));
    JSValueRef s = JSObjectCallAsFunction(ctx, toString, obj, 0, nullptr, &exception);
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), s).As<String>());
    }
    
    return MaybeLocal<String>();
}

/**
 * Returns the name of the function invoked as a constructor for this object.
 */
Local<v8::String> Object::GetConstructorName()
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
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
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), vname)->ToString(context).ToLocalChecked());
    }

    return Local<String>(nullptr);
}

/**
 * Sets the integrity level of the object.
 */
Maybe<bool> Object::SetIntegrityLevel(Local<Context> context, IntegrityLevel level)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(thiz, context);
    
    JSValueRef r;
    LocalException exception(ToIsolateImpl(ToContextImpl(context)));
    if (level == IntegrityLevel::kFrozen) {
        r = exec(ctx, "return _1 === Object.freeze(_1)", 1, &obj, &exception);
    } else {
        r = exec(ctx, "return _1 === Object.seal(_1)", 1, &obj, &exception);
    }

    if (exception.ShouldThrow()) {
        return Nothing<bool>();
    }
    return _maybe<bool>(JSValueToBoolean(ctx, r)).toMaybe();
}

/** Gets the number of internal fields for this Object. */
int Object::InternalFieldCount()
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(this, context);

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ToContextRef(context), obj);
    
    if (IsArrayBufferView() && (!wrap || wrap->m_num_internal_fields < ArrayBufferView::kInternalFieldCount)) {
        // ArrayBufferViews have internal fields by default.  This was created in JS.
        wrap = V82JSC::TrackedObject::makePrivateInstance(ToIsolateImpl(this), ToContextRef(context), obj);
        wrap->m_num_internal_fields = ArrayBufferView::kInternalFieldCount;
        wrap->m_internal_fields_array = JSObjectMakeArray(ToContextRef(context), 0, nullptr, 0);
        JSValueProtect(ToContextRef(context), wrap->m_internal_fields_array);
    }
    return wrap ? wrap->m_num_internal_fields : 0;
}

/** Sets the value in an internal field. */
void Object::SetInternalField(int index, Local<Value> value)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(this, context);

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
    if (!wrap && IsArrayBuffer()) {
        // ArrayBuffers have internal fields by default.  This was created in JS.
        wrap = V82JSC::TrackedObject::makePrivateInstance(ToIsolateImpl(this), ctx, obj);
        wrap->m_num_internal_fields = ArrayBufferView::kInternalFieldCount;
        wrap->m_internal_fields_array = JSObjectMakeArray(ToContextRef(context), 0, nullptr, 0);
        JSValueProtect(ctx, wrap->m_internal_fields_array);
    }
    if (wrap && index < wrap->m_num_internal_fields) {
        JSObjectSetPropertyAtIndex(ctx, wrap->m_internal_fields_array, index, ToJSValueRef(value, context), 0);
    }
}

/**
 * Sets a 2-byte-aligned native pointer in an internal field. To retrieve such
 * a field, GetAlignedPointerFromInternalField must be used, everything else
 * leads to undefined behavior.
 */
void Object::SetAlignedPointerInInternalField(int index, void* value)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(this, context);
    
    if (index < 2) {
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
        if (wrap) {
            wrap->m_embedder_data[index] = value;
        }
    } else {
        SetInternalField(index, External::New(context->GetIsolate(), value));
    }
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
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context)
    };
    LocalException exception(iso);
    JSValueRef has = exec(ctx,
                                  "return Object.prototype.hasOwnProperty.call(_1, _2)",
                                  2, args, &exception);
    if (exception.ShouldThrow()) {
        return Nothing<bool>();
    }
    
    return _maybe<bool>(JSValueToBoolean(ctx, has)).toMaybe();
}
Maybe<bool> Object::HasOwnProperty(Local<Context> context, uint32_t index)
{
    Local<Value> k = Number::New(ToIsolate(ToContextImpl(context)), index);
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
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(this);
    HandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    JSObjectRef raw_object = (JSObjectRef) ToJSValueRef(this, context);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, raw_object);
    if (wrap && wrap->m_proxy_security) {
        raw_object = (JSObjectRef) wrap->m_security;
    }

    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
    (V82JSC::Value::New(ToContextImpl(context), raw_object));

    JSValueRef args[] = {
        ToJSValueRef(thiz, context),
        ToJSValueRef(key, context)
    };
    LocalException exception(iso);
    JSValueRef has = exec(ctx,
                                  "return Object.getOwnPropertyDescriptor(_1, _2) !== undefined",
                                  2, args, &exception);
    if (exception.ShouldThrow()) {
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
    //FIXME: Not sure what this is exactly supposed to do.  Only properties that call back into
    //V8?  Or any property with a getter/setter?
    return HasRealNamedProperty(context, key);
}

/**
 * If result.IsEmpty() no real property was located in the prototype chain.
 * This means interceptors in the prototype chain are not called.
 */
MaybeLocal<v8::Value> Object::GetRealNamedPropertyInPrototypeChain(Local<Context> context,
                                                               Local<Name> key)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    Local<Value> proto = GetPrototype();
    
    MaybeLocal<Value> v = proto.As<Object>()->GetRealNamedProperty(context, key);
    if (!v.IsEmpty()) {
        return scope.Escape(v.ToLocalChecked());
    }
    return MaybeLocal<Value>();
}

/**
 * Gets the property attributes of a real property in the prototype chain,
 * which can be None or any combination of ReadOnly, DontEnum and DontDelete.
 * Interceptors in the prototype chain are not called.
 */
Maybe<v8::PropertyAttribute>
Object::GetRealNamedPropertyAttributesInPrototypeChain(Local<Context> context,
                                                       Local<Name> key)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    Local<Value> proto = GetPrototype();
    return proto.As<Object>()->GetRealNamedPropertyAttributes(context, key);
}

/**
 * If result.IsEmpty() no real property was located on the object or
 * in the prototype chain.
 * This means interceptors in the prototype chain are not called.
 */
MaybeLocal<v8::Value> Object::GetRealNamedProperty(Local<Context> context, Local<Name> key)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    JSContextRef ctx = ToContextRef(context);
    EscapableHandleScope scope(ToIsolate(iso));
    Context::Scope context_scope(context);
    
    Local<Value> thiz = V82JSC::TrackedObject::SecureValue
    (CreateLocal<Value>(&iso->ii, ToImpl<V82JSC::Value>(this)));

    LocalException exception(iso);
    
    JSObjectRef raw_object = (JSObjectRef) ToJSValueRef(thiz, context);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, raw_object);
    if (wrap && wrap->m_proxy_security) {
        raw_object = (JSObjectRef) wrap->m_security;
    }

    JSValueRef args[] = {
        raw_object,
        ToJSValueRef(key, context)
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
    
    JSValueRef value = exec(ctx, code, 2, args, &exception);
    if (exception.ShouldThrow()) {
        if (JSValueIsStrictEqual(ctx, exception.exception_, JSValueMakeNumber(ctx, 0))) {
            exception.Clear();
        }
        return MaybeLocal<Value>();
    }
    
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), value));
}

/**
 * Gets the property attributes of a real property which can be
 * None or any combination of ReadOnly, DontEnum and DontDelete.
 * Interceptors in the prototype chain are not called.
 */
Maybe<v8::PropertyAttribute> Object::GetRealNamedPropertyAttributes(Local<Context> context, Local<Name> key)
{
    JSContextRef ctx = ToContextRef(context);
    HandleScope scope(ToIsolate(this));
    Context::Scope context_scope(context);
    TryCatch try_catch(ToIsolate(this));

    PropertyAttribute retval = PropertyAttribute::None;
    {
        LocalException exception(ToIsolateImpl(this));
        
        JSObjectRef raw_object = (JSObjectRef) ToJSValueRef(this, context);
        auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, raw_object);
        if (wrap && wrap->m_proxy_security) {
            raw_object = (JSObjectRef) wrap->m_security;
        }
        
        Local<Value> thiz = V82JSC::TrackedObject::SecureValue
        (V82JSC::Value::New(ToContextImpl(context), raw_object));
        
        JSValueRef args[] = {
            ToJSValueRef(thiz, context),
            ToJSValueRef(key, context)
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

        JSValueRef value = exec(ctx, code, 2, args, &exception);
        if (!exception.ShouldThrow()) {
            retval = (PropertyAttribute)JSValueToNumber(ctx, value, 0);
        } else {
            JSStringRef err = JSValueToStringCopy(ctx, exception.exception_, 0);
            char e[JSStringGetMaximumUTF8CStringSize(err)];
            JSStringGetUTF8CString(err, e, JSStringGetMaximumUTF8CStringSize(err));
            if (strstr(e, "access denied")) {
                return Nothing<PropertyAttribute>();
            }
            exception.exception_ = 0;
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
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (!wrap) {
        wrap = V82JSC::TrackedObject::makePrivateInstance(ToIsolateImpl(this), ctx, (JSObjectRef)obj);
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
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(thiz, context);
    JSValueRef newobj = exec(ctx, "return {..._1}", 1, &obj);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), newobj).As<Object>());
}

/**
 * Returns the context in which the object was created.
 */
Local<v8::Context> Object::CreationContext()
{
    auto isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    auto obj = ToImpl<V82JSC::Value, Object>(this);
    auto iso = ToIsolateImpl(obj);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ToContextRef(iso->m_nullContext.Get(isolate)),
                                                 (JSObjectRef)obj->m_value);
#ifdef USE_JAVASCRIPTCORE_PRIVATE_API
    JSGlobalContextRef ctx = JSCPrivate::JSObjectGetGlobalContext((JSObjectRef)(wrap ? wrap->m_security : obj->m_value));
    CHECK_EQ(1, iso->m_global_contexts.count(ctx));
    return scope.Escape(iso->m_global_contexts[ctx].Get(ToIsolate(iso)));
#else
    auto context = isolate->GetCurrentContext();
    if (context.IsEmpty()) {
        context = iso->m_nullContext.Get(isolate);
    }
    auto gCtx = FindGlobalContext(context);
    JSGlobalContextRef ctx = (JSGlobalContextRef) ToContextRef(gCtx);
    CHECK_EQ(1, iso->m_global_contexts.count(ctx));
    return scope.Escape(iso->m_global_contexts[ctx].Get(ToIsolate(iso)));
#endif
}

/**
 * Checks whether a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 * When an Object is callable this method returns true.
 */
bool Object::IsCallable()
{
    if (IsFunction()) return true;
    
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)obj);
    if (!wrap) return false;
    Local<ObjectTemplate> templ = wrap->m_object_template.Get(isolate);
    if (templ.IsEmpty()) return false;
    return ToImpl<V82JSC::ObjectTemplate>(templ)->m_callback != nullptr;
}

/**
 * True if this object is a constructor.
 */
bool Object::IsConstructor()
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef<Value>(this, context);
    return JSValueToBoolean(ctx, exec(ctx,
                                              "try {Reflect.construct(String,[],_1);} catch(e) { return false; } return true",
                                              1, &obj));
}

/**
 * Call an Object as a function if a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 */
MaybeLocal<v8::Value> Object::CallAsFunction(Local<Context> context,
                                         Local<Value> recv,
                                         int argc,
                                         Local<Value> argv[])
{
    Function *f = static_cast<Function *>(this);
    return f->Call(context, recv, argc, argv);
}

/**
 * Call an Object as a constructor if a callback is set by the
 * ObjectTemplate::SetCallAsFunctionHandler method.
 * Note: This method behaves like the Function::NewInstance method.
 */
MaybeLocal<v8::Value> Object::CallAsConstructor(Local<Context> context,
                                            int argc, Local<Value> argv[])
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    Local<Value> thiz = CreateLocal<Value>(isolate, ToImpl<V82JSC::Value>(this));
    thiz = V82JSC::TrackedObject::SecureValue(thiz);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef func = (JSObjectRef) ToJSValueRef(thiz, context);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    JSValueRef args[argc+1];
    args[0] = func;
    for (int i=0; i<argc; i++) {
        args[i+1] = ToJSValueRef<Value>(argv[i], context);
    }
    LocalException exception(iso);
    JSValueRef newobj = exec(ctx, "return new _1(...Array.prototype.slice.call(arguments, 1))", argc+1, args, &exception);
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), newobj).As<Object>());
    }
    return MaybeLocal<Value>();
}

Local<Object> Object::New(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = JSObjectMake(ctx, 0, 0);
    
    Local<Value> o = scope.Escape(V82JSC::Value::New(ToContextImpl(context), obj));
    return o.As<Object>();
}

void* Object::SlowGetAlignedPointerFromInternalField(int index)
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(this, context);

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
    if (index < v8::ArrayBufferView::kInternalFieldCount && (IsTypedArray() || IsDataView())) {
        JSStringRef buffer = JSStringCreateWithUTF8CString("buffer");
        obj = (JSObjectRef) JSObjectGetProperty(ctx, obj, buffer, 0);
        JSStringRelease(buffer);
        wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
    }

    if (wrap && index < wrap->m_num_internal_fields) {
        if (index < 2) {
            return wrap->m_embedder_data[index];
        } else {
            Local<Value> external = V82JSC::Value::New(ToContextImpl(context),
                    JSObjectGetPropertyAtIndex(ctx, wrap->m_internal_fields_array, index, 0));
            if (external->IsExternal()) {
                return external.As<External>()->Value();
            }
        }
    }
    return nullptr;
}

Local<v8::Value> Object::SlowGetInternalField(int index)
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);

    JSObjectRef obj = (JSObjectRef) ToJSValueRef(this, context);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
    if (index < v8::ArrayBufferView::kInternalFieldCount && (IsTypedArray() || IsDataView())) {
        JSStringRef buffer = JSStringCreateWithUTF8CString("buffer");
        obj = (JSObjectRef) JSObjectGetProperty(ctx, obj, buffer, 0);
        JSStringRelease(buffer);
        wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, obj);
    }
    if (wrap && index < wrap->m_num_internal_fields) {
        Local<Value> r = V82JSC::Value::New(ToContextImpl(context),
                                        JSObjectGetPropertyAtIndex(ctx, wrap->m_internal_fields_array, index, 0));
        return scope.Escape(r);
    }
    return Local<Value>();
}

/**
 * If this object is a Set, Map, WeakSet or WeakMap, this returns a
 * representation of the elements of this object as an array.
 * If this object is a SetIterator or MapIterator, this returns all
 * elements of the underlying collection, starting at the iterator's current
 * position.
 * For other types, this will return an empty MaybeLocal<Array> (without
 * scheduling an exception).
 */
MaybeLocal<v8::Array> Object::PreviewEntries(bool* is_key_value)
{
    static const char *code =
    "let m = (new Map())[Symbol.iterator](); "
    "let s = (new Set())[Symbol.iterator](); "
    "let is = typeof _1 === 'object' && (s.isPrototypeOf(_1) || m.isPrototypeOf(_1) || _1 instance of Set || _1 instanceof Map || _1 instance of WeakMap || _1 instanceof WeakSet); "
    "if (is) return Array.from(_1); else return undefined;";
    
    auto isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    auto context = ToCurrentContext(this);
    auto ctx = ToContextRef(context);
    auto value = ToJSValueRef(this, context);
    auto rval = exec(ctx, code, 1, &value);
    if (JSValueIsObject(ctx, rval)) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), rval).As<Array>());
    } else {
        return MaybeLocal<Array>();
    }
}


