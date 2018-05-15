//
//  Value.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<Value> ValueImpl::New(const ContextImpl *ctx, JSValueRef value)
{
    JSType t = JSValueGetType(ctx->m_ctxRef, value);
    v8::internal::InstanceType instancet = v8::internal::JS_VALUE_TYPE;
    IsolateImpl* isolateimpl = V82JSC::ToIsolateImpl(ctx);
    v8::Isolate *isolate = V82JSC::ToIsolate(isolateimpl);
    
    double num = 0.0;
    switch (t) {
        case kJSTypeUndefined: {
            return isolateimpl->m_undefined.Get(isolate);
        }
        case kJSTypeNull: {
            return isolateimpl->m_null.Get(isolate);
        }
        case kJSTypeBoolean: {
            if (JSValueToBoolean(ctx->m_ctxRef, value))
                return isolateimpl->m_yup.Get(isolate);
            else
                return isolateimpl->m_nope.Get(isolate);
        }
        case kJSTypeString: {
            instancet = v8::internal::STRING_TYPE;
            break;
        }
        case kJSTypeNumber: {
            num = JSValueToNumber(ctx->m_ctxRef, value, 0);
            double intpart;
            if (value != isolateimpl->m_negative_zero && modf(num, &intpart) == 0.0) {
                if (internal::Smi::IsValid(intpart)) {
                    return V82JSC::CreateLocalSmi<Value>(reinterpret_cast<internal::Isolate*>(isolateimpl),
                                                         internal::Smi::FromInt(intpart));
                }
            }
            instancet = v8::internal::HEAP_NUMBER_TYPE;
        }
        default:
            break;
    }
    
    ValueImpl * impl = static_cast<ValueImpl *>(HeapAllocator::Alloc(isolateimpl, sizeof(ValueImpl), valueDestructor));
    impl->m_value = value;
    impl->m_creationCtx = JSContextGetGlobalContext(ctx->m_ctxRef);
    JSValueProtect(ctx->m_ctxRef, impl->m_value);
    if (t == kJSTypeNumber) {
        reinterpret_cast<internal::HeapNumber*>(V82JSC::Map(impl))->set_value(num);
    }
    V82JSC::Map(impl)->set_instance_type(instancet);

    return V82JSC::CreateLocal<Value>(V82JSC::ToIsolate(isolateimpl), impl);
}

#define FROMTHIS(c,v) \
    auto cc = V82JSC::ToCurrentContext(this); \
    auto c = V82JSC::ToContextImpl(cc); \
    auto v = V82JSC::ToJSValueRef<Value>(this, cc)

/**
 * Returns true if this value is true.
 */
bool Value::IsTrue() const { FROMTHIS(c,v); return JSValueIsStrictEqual(c->m_ctxRef, v, JSValueMakeBoolean(c->m_ctxRef, true)); }

/**
 * Returns true if this value is false.
 */
bool Value::IsFalse() const { FROMTHIS(c,v); return JSValueIsStrictEqual(c->m_ctxRef, v, JSValueMakeBoolean(c->m_ctxRef, false)); }

/**
 * Returns true if this value is a symbol or a string.
 */
bool Value::IsName() const { return IsString() || IsSymbol(); }

/**
 * Returns true if this value is a symbol.
 */
bool Value::IsSymbol() const { return IS(IsSymbol, "return typeof _1 === 'symbol'"); }

/**
 * Returns true if this value is a function.
 */
bool Value::IsFunction() const { return IS(IsFunction, "return typeof _1 === 'function'"); }

/**
 * Returns true if this value is an array. Note that it will return false for
 * an Proxy for an array.
 */
bool Value::IsArray() const { FROMTHIS(c,v); return JSValueIsArray(c->m_ctxRef, v); }

/**
 * Returns true if this value is an object.
 */
bool Value::IsObject() const { FROMTHIS(c,v); return JSValueIsObject(c->m_ctxRef, v); }

/**
 * Returns true if this value is boolean.
 */
bool Value::IsBoolean() const { FROMTHIS(c,v); return JSValueIsBoolean(c->m_ctxRef, v); }

/**
 * Returns true if this value is a number.
 */
bool Value::IsNumber() const { FROMTHIS(c,v); return JSValueIsNumber(c->m_ctxRef, v); }

/**
 * Returns true if this value is external.
 */
bool Value::IsExternal() const { return IS(IsExternal, "return Object.prototype.toString.call( _1 ) === '[object External]';"); }

/**
 * Returns true if this value is a 32-bit signed integer.
 */
bool Value::IsInt32() const
{
    if (IsNumber()) {
        JSValueRef exception = nullptr;
        FROMTHIS(c,v);
        double number = JSValueToNumber(c->m_ctxRef, v, &exception);
        double intpart;
        if (v == V82JSC::ToIsolateImpl(c)->m_negative_zero) return false;
        if (std::modf(number, &intpart) == 0.0) {
            return (intpart >= std::numeric_limits<std::int32_t>::min() && intpart <= std::numeric_limits<std::int32_t>::max());
        }
    }
    return false;
}

/**
 * Returns true if this value is a 32-bit unsigned integer.
 */
bool Value::IsUint32() const
{
    if (IsNumber()) {
        JSValueRef exception = nullptr;
        FROMTHIS(c,v);
        double number = JSValueToNumber(c->m_ctxRef, v, &exception);
        double intpart;
        if (v == V82JSC::ToIsolateImpl(c)->m_negative_zero) return false;
        if (std::modf(number, &intpart) == 0.0) {
            return (intpart >= 0 && intpart <= std::numeric_limits<std::uint32_t>::max());
        }
    }
    return false;
}

/**
 * Returns true if this value is a Date.
 */
bool Value::IsDate() const { FROMTHIS(c,v); return JSValueIsDate(c->m_ctxRef, v); }

/**
 * Returns true if this value is an Arguments object.
 */
bool Value::IsArgumentsObject() const { return IS(IsArgumentsObject, "return Object.prototype.toString.call( _1 ) === '[object Arguments]';"); }

/**
 * Returns true if this value is a Boolean object.
 */
bool Value::IsBooleanObject() const { return IS(IsBooleanObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'boolean');"); }

/**
 * Returns true if this value is a Number object.
 */
bool Value::IsNumberObject() const { return IS(IsNumberObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'number');"); }

/**
 * Returns true if this value is a String object.
 */
bool Value::IsStringObject() const { return IS(IsStringObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'string');"); }

/**
 * Returns true if this value is a Symbol object.
 */
bool Value::IsSymbolObject() const { return IS(IsSymbolObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'symbol');"); }

/**
 * Returns true if this value is a NativeError.
 */
bool Value::IsNativeError() const { return IS(IsNativeError, "return _1 instanceof Error"); }

/**
 * Returns true if this value is a RegExp.
 */
bool Value::IsRegExp() const { return IS(IsRegExp, "return Object.prototype.toString.call( _1 ) === '[object RegExp]';);"); }

/**
 * Returns true if this value is an async function.
 */
bool Value::IsAsyncFunction() const { return IS(IsAsyncFunction, "return _1 && _1.constructor && _1.constructor.name === 'AsyncFunction';"); }

/**
 * Returns true if this value is a Generator function.
 */
bool Value::IsGeneratorFunction() const { return IS(IsGeneratorFunction, "var Generator = (function*(){}).constructor; return _1 instanceof Generator"); }

/**
 * Returns true if this value is a Generator object (iterator).
 */
bool Value::IsGeneratorObject() const { return IS(IsGeneratorObject, "return _1 && typeof _1[Symbol.iterator] === 'function'"); }

/**
 * Returns true if this value is a Promise.
 */
bool Value::IsPromise() const { return IS(IsPromise, "return _1 && Promise && Promise.resolve && Promise.resolve(_1) == _1"); }

/**
 * Returns true if this value is a Map.
 */
bool Value::IsMap() const { return IS(IsMap, "return _1 instanceof Map"); }

/**
 * Returns true if this value is a Set.
 */
bool Value::IsSet() const { return IS(IsSet, "return _1 instanceof Set"); }

/**
 * Returns true if this value is a Map Iterator.
 */
bool Value::IsMapIterator() const { return false; } // FIXME

/**
 * Returns true if this value is a Set Iterator.
 */
bool Value::IsSetIterator() const { return false; } // FIXME

/**
 * Returns true if this value is a WeakMap.
 */
bool Value::IsWeakMap() const { return IS(IsWeakMap, "return _1 instanceof WeakMap"); }

/**
 * Returns true if this value is a WeakSet.
 */
bool Value::IsWeakSet() const { return IS(IsWeakSet, "return _1 instanceof WeakSet"); }

/**
 * Returns true if this value is an ArrayBuffer.
 */
bool Value::IsArrayBuffer() const { return IS(IsArrayBuffer, "return _1 instanceof ArrayBuffer"); }

/**
 * Returns true if this value is an ArrayBufferView.
 */
bool Value::IsArrayBufferView() const { return IS(IsArrayBufferView, "return _1 && _1.buffer instanceof ArrayBuffer && _1.byteLength !== undefined"); }

/**
 * Returns true if this value is one of TypedArrays.
 */
bool Value::IsTypedArray() const { return IS(IsTypedArray, "return _1 && ArrayBuffer.isView(_1) && Object.prototype.toString.call(_1) !== '[object DataView]'"); }

/**
 * Returns true if this value is an Uint8Array.
 */
bool Value::IsUint8Array() const { return IS(IsUint8Array, "return _1 instanceof Uint8Array"); }

/**
 * Returns true if this value is an Uint8ClampedArray.
 */
bool Value::IsUint8ClampedArray() const { return IS(IsUint8ClampedArray, "return _1 instanceof Uint8ClampedArray"); }

/**
 * Returns true if this value is an Int8Array.
 */
bool Value::IsInt8Array() const { return IS(IsInt8Array, "return _1 instanceof Int8Array"); }

/**
 * Returns true if this value is an Uint16Array.
 */
bool Value::IsUint16Array() const { return IS(IsUint16Array, "return _1 instanceof Uint16Array"); }

/**
 * Returns true if this value is an Int16Array.
 */
bool Value::IsInt16Array() const { return IS(IsInt16Array, "return _1 instanceof Int16Array"); }

/**
 * Returns true if this value is an Uint32Array.
 */
bool Value::IsUint32Array() const { return IS(IsUint32Array, "return _1 instanceof Uint32Array"); }

/**
 * Returns true if this value is an Int32Array.
 */
bool Value::IsInt32Array() const { return IS(IsInt32Array, "return _1 instanceof Int32Array"); }

/**
 * Returns true if this value is a Float32Array.
 */
bool Value::IsFloat32Array() const { return IS(IsFloat32Array, "return _1 instanceof Float32Array"); }

/**
 * Returns true if this value is a Float64Array.
 */
bool Value::IsFloat64Array() const { return IS(IsFloat64Array, "return _1 instanceof Float64Array"); }

/**
 * Returns true if this value is a DataView.
 */
bool Value::IsDataView() const { return IS(IsDataView, "return _1 && Object.prototype.toString.call(_1) === '[object DataView]'"); }

/**
 * Returns true if this value is a SharedArrayBuffer.
 * This is an experimental feature.
 */
bool Value::IsSharedArrayBuffer() const { return false; } // FIXME

/**
 * Returns true if this value is a JavaScript Proxy.
 */
bool Value::IsProxy() const { return false; } // FIXME

bool Value::IsWebAssemblyCompiledModule() const { return false; } // FIXME

template <typename T, typename F>
Maybe<T> handleException(IsolateImpl* isolate, F&& lambda)
{
    LocalException exception(isolate);
    T value = lambda(&exception);
    if (!exception.ShouldThow()) {
        return _maybe<T>(value).toMaybe();
    }
    return Nothing<T>();
}

template <typename T>
Maybe<T> toValue(const Value* thiz, Local<Context> context)
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    JSValueRef value = V82JSC::ToJSValueRef<Value>(thiz, context);
    IsolateImpl* i = V82JSC::ToIsolateImpl(ctx);

    LocalException exception(i);
    T ret;
    if (std::is_same<T,bool>::value) {
        ret = JSValueToBoolean(ctx->m_ctxRef, value);
    } else {
        double number = JSValueToNumber(ctx->m_ctxRef, value, &exception);
        if (std::isnan(number)) number = 0;
        ret = static_cast<T>(number);
    }
    if (!exception.ShouldThow()) {
        return _maybe<T>(ret).toMaybe();
    }
    return Nothing<T>();
}
Maybe<bool> Value::BooleanValue(Local<Context> context) const    { return toValue<bool>(this, context); }
Maybe<double> Value::NumberValue(Local<Context> context) const   { return toValue<double>(this, context); }
Maybe<int64_t> Value::IntegerValue(Local<Context> context) const { return toValue<int64_t>(this, context); }
Maybe<uint32_t> Value::Uint32Value(Local<Context> context) const { return toValue<uint32_t>(this, context); }
Maybe<int32_t> Value::Int32Value(Local<Context> context) const   { return toValue<int32_t>(this, context); }

Maybe<bool> Value::Equals(Local<Context> context, Local<Value> that) const
{
    JSValueRef this_ = V82JSC::ToJSValueRef<Value>(this, context);
    JSValueRef that_ = V82JSC::ToJSValueRef<Value>(that, context);
    JSContextRef context_ = V82JSC::ToContextRef(context);
    IsolateImpl* i = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));

    LocalException exception(i);
    bool is = JSValueIsEqual(context_, this_, that_, &exception);
    if (!exception.ShouldThow()) {
        return _maybe<bool>(is).toMaybe();
    }
    return Nothing<bool>();
}
bool Value::StrictEquals(Local<Value> that) const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef this_ = V82JSC::ToJSValueRef(this, context);
    JSValueRef that_ = V82JSC::ToJSValueRef(that, context);
    return JSValueIsStrictEqual(V82JSC::ToContextRef(context), this_, that_);
}
bool Value::SameValue(Local<Value> that) const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef this_ = V82JSC::ToJSValueRef(this, context);
    JSValueRef that_ = V82JSC::ToJSValueRef(that, context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));

    if (this_ == that_) return true;
    if (this_ == iso->m_negative_zero || that_ == iso->m_negative_zero) {
        return false;
    }
    return JSValueIsStrictEqual(V82JSC::ToContextRef(context), this_, that_);
}

Local<String> Value::TypeOf(Isolate* isolate)
{
    FROMTHIS(c,v);
    JSValueRef exception = nullptr;
    JSValueRef to = V82JSC::exec(c->m_ctxRef, "return typeof _1", 1, &v);
    return ValueImpl::New(isolate, JSValueToStringCopy(c->m_ctxRef, to, &exception));
}

Maybe<bool> Value::InstanceOf(Local<Context> context, Local<Object> object)
{
    FROMTHIS(c,v);
    JSValueRef args[] = {
        v,
        V82JSC::ToJSValueRef(object, context)
    };
    LocalException exception(V82JSC::ToIsolateImpl(this));
    JSValueRef is = V82JSC::exec(c->m_ctxRef, "return _1 instanceof _2", 2, args, &exception);
    if (exception.ShouldThow()) {
        return Nothing<bool>();
    }
    return _maybe<bool>(JSValueToBoolean(c->m_ctxRef, is)).toMaybe();
}

MaybeLocal<Uint32> Value::ToArrayIndex(Local<Context> context) const
{
    MaybeLocal<Number> num = ToNumber(context);
    if (!num.IsEmpty() && num.ToLocalChecked()->IsUint32()) {
        return ToUint32(context);
    }
    return MaybeLocal<Uint32>();
}

MaybeLocal<v8::Boolean> Value::ToBoolean(Local<Context> context) const
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    JSValueRef v = V82JSC::ToJSValueRef(this, context);
    return ValueImpl::New(ctx, JSValueMakeBoolean(ctx->m_ctxRef, JSValueToBoolean(ctx->m_ctxRef, v))).As<v8::Boolean>();
}
MaybeLocal<String> Value::ToString(Local<Context> context) const
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));

    JSValueRef v = V82JSC::ToJSValueRef(this, context);
    LocalException exception(iso);
    JSStringRef s = JSValueToStringCopy (ctx->m_ctxRef, v, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<String>();
    }
    return ValueImpl::New(reinterpret_cast<Isolate*>(iso), s);
}
MaybeLocal<String> Value::ToDetailString(Local<Context> context) const { return ToString(context); } // FIXME
MaybeLocal<Object> Value::ToObject(Local<Context> context) const
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));

    JSValueRef v = V82JSC::ToJSValueRef(this, context);
    LocalException exception(iso);
    JSObjectRef o = JSValueToObject(ctx->m_ctxRef, v, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<Object>();
    }
    return ValueImpl::New(ctx, o).As<Object>();
}

template<class T, typename C>
MaybeLocal<T> ToNum(const Value* thiz, Local<Context> context)
{
    ContextImpl *ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));
    JSValueRef v = V82JSC::ToJSValueRef(thiz, context);
    LocalException exception(iso);
    double num = JSValueToNumber(ctx->m_ctxRef, v, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<T>();
    }
    C val;
    if (std::is_same<C, double>::value ) {
        val = num;
    } else {
        int64_t ival = static_cast<int64_t>(num);
        uint32_t uval = ival & 0xffffffff;
        val = *reinterpret_cast<C*>(&uval);
    }
    Local<Number> nval = Number::New(reinterpret_cast<Isolate*>(iso), val);
    return nval.As<T>();
}

MaybeLocal<Number> Value::ToNumber(Local<Context> context) const
{
    return ToNum<Number, double>(this, context);
}
MaybeLocal<Integer> Value::ToInteger(Local<Context> context) const
{
    return ToNum<Integer, int32_t>(this, context);
}
MaybeLocal<Uint32> Value::ToUint32(Local<Context> context) const
{
    return ToNum<Uint32, uint32_t>(this, context);
}
MaybeLocal<Int32> Value::ToInt32(Local<Context> context) const
{
    return ToNum<Int32, int32_t>(this, context);
}

JSClassRef s_externalClass = nullptr;

Local<External> External::New(Isolate* isolate, void* value)
{
    if (!s_externalClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.attributes |= kJSClassAttributeNoAutomaticPrototype;
        definition.className = "External";
        s_externalClass = JSClassCreate(&definition);
    }
    
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSObjectRef external = JSObjectMake(V82JSC::ToContextRef(context), s_externalClass, value);
    auto e = ValueImpl::New(V82JSC::ToContextImpl(context), external);
    
    return * reinterpret_cast<Local<External> *>(&e);
}

void* External::Value() const
{
    auto context = V82JSC::ToCurrentContext(this);
    auto v = V82JSC::ToJSValueRef<External>(this, context);
    return JSObjectGetPrivate((JSObjectRef)v);
}
