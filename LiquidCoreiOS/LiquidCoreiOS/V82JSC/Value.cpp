//
//  Value.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<Value> ValueImpl::New(ContextImpl *ctx, JSValueRef value)
{
    ValueImpl * impl = (ValueImpl *) malloc(sizeof(ValueImpl));
    memset(impl, 0, sizeof(ValueImpl));
    impl->pMap = (v8::internal::Map *)((reinterpret_cast<intptr_t>(&impl->map) & ~3) + 1);
    impl->pMap->set_instance_type(v8::internal::JS_VALUE_TYPE);

    impl->m_context = ctx;
    impl->m_value = value;
    
    Local<Value> v;
    *(reinterpret_cast<Value**>(&v)) = impl;
    return v;
}

#define THIS (const_cast<ValueImpl*>(static_cast<const ValueImpl*>(this)))
#define JSFUNC(name_,code_) \
([&]() { \
  if (!THIS->m_context->IsFunctionRefs[ContextImpl::IsFunctions::name_]) { \
    JSStringRef name = JSStringCreateWithUTF8CString(#name_); \
    JSStringRef param = JSStringCreateWithUTF8CString("v"); \
    JSStringRef body = JSStringCreateWithUTF8CString(code_); \
    JSValueRef exception; \
    THIS->m_context->IsFunctionRefs[ContextImpl::IsFunctions::name_] = JSObjectMakeFunction(THIS->m_context->m_context, name, 1, &param, body, 0, 0, &exception); \
    JSValueProtect(THIS->m_context->m_context, THIS->m_context->IsFunctionRefs[ContextImpl::IsFunctions::name_]); \
    JSStringRelease(name); \
    JSStringRelease(param); \
    JSStringRelease(body); \
  } \
  return THIS->m_context->IsFunctionRefs[ContextImpl::IsFunctions::name_]; \
})()

#define IS(name_,code_) \
([&](){ \
  JSValueRef exception; \
  return JSValueToBoolean(THIS->m_context->m_context, \
    JSObjectCallAsFunction(THIS->m_context->m_context, JSFUNC(name_, code_), 0, 1, &THIS->m_value, &exception)); \
})()

/**
 * Returns true if this value is true.
 */
bool Value::IsTrue() const { return JSValueToBoolean(THIS->m_context->m_context, THIS->m_value); }

/**
 * Returns true if this value is false.
 */
bool Value::IsFalse() const { return !IsTrue(); }

/**
 * Returns true if this value is a symbol or a string.
 */
bool Value::IsName() const { return IsString() || IsSymbol(); }

/**
 * Returns true if this value is a symbol.
 */
bool Value::IsSymbol() const { return IS(IsSymbol, "return typeof v === 'symbol'"); }

/**
 * Returns true if this value is a function.
 */
bool Value::IsFunction() const { return IS(IsFunction, "return typeof v === 'function'"); }

/**
 * Returns true if this value is an array. Note that it will return false for
 * an Proxy for an array.
 */
bool Value::IsArray() const { return JSValueIsArray(THIS->m_context->m_context, THIS->m_value); }

/**
 * Returns true if this value is an object.
 */
bool Value::IsObject() const { return JSValueIsObject(THIS->m_context->m_context, THIS->m_value); }

/**
 * Returns true if this value is boolean.
 */
bool Value::IsBoolean() const { return JSValueIsBoolean(THIS->m_context->m_context, THIS->m_value); }

/**
 * Returns true if this value is a number.
 */
bool Value::IsNumber() const { return JSValueIsNumber(THIS->m_context->m_context, THIS->m_value); }

/**
 * Returns true if this value is external.
 */
bool Value::IsExternal() const { return IS(IsExternal, "return Object.prototype.toString.call( v ) === '[object External]';);"); }

/**
 * Returns true if this value is a 32-bit signed integer.
 */
bool Value::IsInt32() const
{
    if (IsNumber()) {
        JSValueRef exception;
        double number = JSValueToNumber(THIS->m_context->m_context, THIS->m_value, &exception);
        double intpart;
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
        JSValueRef exception;
        double number = JSValueToNumber(THIS->m_context->m_context, THIS->m_value, &exception);
        double intpart;
        if (std::modf(number, &intpart) == 0.0) {
            return (intpart >= 0 && intpart <= std::numeric_limits<std::uint32_t>::max());
        }
    }
    return false;
}

/**
 * Returns true if this value is a Date.
 */
bool Value::IsDate() const { return JSValueIsDate(THIS->m_context->m_context, THIS->m_value); }

/**
 * Returns true if this value is an Arguments object.
 */
bool Value::IsArgumentsObject() const { return IS(IsArgumentsObject, "return Object.prototype.toString.call( v ) === '[object Arguments]';);"); }

/**
 * Returns true if this value is a Boolean object.
 */
bool Value::IsBooleanObject() const { return IS(IsBooleanObject, "(typeof v === 'object' && v !== null && typeof v.valueOf() === 'boolean');"); }

/**
 * Returns true if this value is a Number object.
 */
bool Value::IsNumberObject() const { return IS(IsNumberObject, "(typeof v === 'object' && v !== null && typeof v.valueOf() === 'number');"); }

/**
 * Returns true if this value is a String object.
 */
bool Value::IsStringObject() const { return IS(IsStringObject, "(typeof v === 'object' && v !== null && typeof v.valueOf() === 'string');"); }

/**
 * Returns true if this value is a Symbol object.
 */
bool Value::IsSymbolObject() const { return IS(IsSymbolObject, "(typeof v === 'object' && v !== null && typeof v.valueOf() === 'symbol');"); }

/**
 * Returns true if this value is a NativeError.
 */
bool Value::IsNativeError() const { return false; } // FIXME

/**
 * Returns true if this value is a RegExp.
 */
bool Value::IsRegExp() const { return IS(IsRegExp, "return Object.prototype.toString.call( v ) === '[object RegExp]';);"); }

/**
 * Returns true if this value is an async function.
 */
bool Value::IsAsyncFunction() const { return IS(IsAsyncFunction, "return v && v.constructor && v.constructor.name === 'AsyncFunction';"); }

/**
 * Returns true if this value is a Generator function.
 */
bool Value::IsGeneratorFunction() const { return IS(IsGeneratorFunction, "var Generator = (function*(){}).constructor; return v instanceof Generator"); }

/**
 * Returns true if this value is a Generator object (iterator).
 */
bool Value::IsGeneratorObject() const { return IS(IsGeneratorObject, "return v && typeof v[Symbol.iterator] === 'function'"); }

/**
 * Returns true if this value is a Promise.
 */
bool Value::IsPromise() const { return IS(IsPromise, "return v && Promise && Promise.resolve && Promise.resolve(v) == v"); }

/**
 * Returns true if this value is a Map.
 */
bool Value::IsMap() const { return IS(IsMap, "return v instanceof Map"); }

/**
 * Returns true if this value is a Set.
 */
bool Value::IsSet() const { return IS(IsSet, "return v instanceof Set"); }

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
bool Value::IsWeakMap() const { return IS(IsWeakMap, "return v instanceof WeakMap"); }

/**
 * Returns true if this value is a WeakSet.
 */
bool Value::IsWeakSet() const { return IS(IsWeakSet, "return v instanceof WeakSet"); }

/**
 * Returns true if this value is an ArrayBuffer.
 */
bool Value::IsArrayBuffer() const { return IS(IsArrayBuffer, "return v instanceof ArrayBuffer"); }

/**
 * Returns true if this value is an ArrayBufferView.
 */
bool Value::IsArrayBufferView() const { return IS(IsArrayBufferView, "return v && v.buffer instanceof ArrayBuffer && v.byteLength !== undefined"); }

/**
 * Returns true if this value is one of TypedArrays.
 */
bool Value::IsTypedArray() const { return IS(IsTypedArray, "return v && ArrayBuffer.isView(v) && Object.prototype.toString.call(v) !== '[object DataView]'"); }

/**
 * Returns true if this value is an Uint8Array.
 */
bool Value::IsUint8Array() const { return IS(IsUint8Array, "return v instanceof Uint8Array"); }

/**
 * Returns true if this value is an Uint8ClampedArray.
 */
bool Value::IsUint8ClampedArray() const { return IS(IsUint8ClampedArray, "return v instanceof Uint8ClampedArray"); }

/**
 * Returns true if this value is an Int8Array.
 */
bool Value::IsInt8Array() const { return IS(IsInt8Array, "return v instanceof Int8Array"); }

/**
 * Returns true if this value is an Uint16Array.
 */
bool Value::IsUint16Array() const { return IS(IsUint16Array, "return v instanceof Uint16Array"); }

/**
 * Returns true if this value is an Int16Array.
 */
bool Value::IsInt16Array() const { return IS(IsInt16Array, "return v instanceof Int16Array"); }

/**
 * Returns true if this value is an Uint32Array.
 */
bool Value::IsUint32Array() const { return IS(IsUint32Array, "return v instanceof Uint32Array"); }

/**
 * Returns true if this value is an Int32Array.
 */
bool Value::IsInt32Array() const { return IS(IsInt32Array, "return v instanceof Int32Array"); }

/**
 * Returns true if this value is a Float32Array.
 */
bool Value::IsFloat32Array() const { return IS(IsFloat32Array, "return v instanceof Float32Array"); }

/**
 * Returns true if this value is a Float64Array.
 */
bool Value::IsFloat64Array() const { return IS(IsFloat64Array, "return v instanceof Float64Array"); }

/**
 * Returns true if this value is a DataView.
 */
bool Value::IsDataView() const { return IS(IsDataView, "return v && Object.prototype.toString.call(v) !== '[object DataView]'"); }

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

template <class T>
class _maybe {
public:
    bool has_value_;
    T value_;
};

template <typename T, typename F>
Maybe<T> handleException(F&& lambda)
{
    JSValueRef exception = nullptr;
    T value = lambda(&exception);
    if (!exception) {
        _maybe<T> maybe;
        maybe.has_value_ = true;
        maybe.value_ = value;
        return *(reinterpret_cast<Maybe<T> *>(&maybe));
    }
    return Nothing<T>();
}

template <typename T>
Maybe<T> toValue(const Value* thiz, Local<Context> context)
{
    ValueImpl *impl = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(thiz));
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleException<T>([ctx,impl](JSValueRef *exception) -> T {
        if (std::is_same<T,bool>::value) {
            return JSValueToBoolean(ctx->m_context, impl->m_value);
        } else {
            return (T) JSValueToNumber(ctx->m_context, impl->m_value, exception);
        }
    });
}
Maybe<bool> Value::BooleanValue(Local<Context> context) const    { return toValue<bool>(this, context); }
Maybe<double> Value::NumberValue(Local<Context> context) const   { return toValue<double>(this, context); }
Maybe<int64_t> Value::IntegerValue(Local<Context> context) const { return toValue<int64_t>(this, context); }
Maybe<uint32_t> Value::Uint32Value(Local<Context> context) const { return toValue<uint32_t>(this, context); }
Maybe<int32_t> Value::Int32Value(Local<Context> context) const   { return toValue<int32_t>(this, context); }

Maybe<bool> Value::Equals(Local<Context> context, Local<Value> that) const
{
    ValueImpl *this_ = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(this));
    ValueImpl *that_ = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(*that));
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleException<bool>([ctx,this_,that_](JSValueRef *exception) {
        return JSValueIsEqual(ctx->m_context, this_->m_value, that_->m_value, exception);
    });
}
bool Value::StrictEquals(Local<Value> that) const
{
    ValueImpl *this_ = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(this));
    ValueImpl *that_ = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(*that));
    return JSValueIsStrictEqual(this_->m_context->m_context, this_->m_value, that_->m_value);
}
bool Value::SameValue(Local<Value> that) const
{
    return this == *that;
}

Local<String> Value::TypeOf(Isolate*)
{
    JSValueRef exception;
    return StringImpl::New(JSValueToStringCopy(THIS->m_context->m_context,
                                               JSFUNC(TypeOf, "return typeof v"), &exception));
}

Maybe<bool> Value::InstanceOf(Local<Context> context, Local<Object> object) { return Nothing<bool>(); }

MaybeLocal<Uint32> Value::ToArrayIndex(Local<Context> context) const { return MaybeLocal<Uint32>(); }

template <class T>
class _local {
public:
    T* val_;
    Local<T> toLocal() { return *(reinterpret_cast<Local<T> *>(this)); }
};

template <typename T, typename F>
MaybeLocal<T> handleLocalException(const Value* value, F&& lambda)
{
    JSValueRef exception = nullptr;
    lambda(&exception);
    if (!exception) {
        _local<T> local;
        local.val_ = static_cast<T*>(const_cast<Value*>(value));
        return MaybeLocal<T>(local.toLocal());
    }
    return MaybeLocal<T>();
}

MaybeLocal<v8::Boolean> Value::ToBoolean(Local<Context> context) const
{
    _local<Boolean> local;
    local.val_ = static_cast<Boolean*>(const_cast<Value*>(this));
    return MaybeLocal<Boolean>(local.toLocal());
}
MaybeLocal<Number> Value::ToNumber(Local<Context> context) const
{
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleLocalException<Number>(this, [&](JSValueRef *exception) {
        JSValueToNumber(ctx->m_context, THIS->m_value, exception);
    });
}
MaybeLocal<String> Value::ToString(Local<Context> context) const
{
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleLocalException<String>(this, [&](JSValueRef *exception) {
        JSStringRef s = JSValueToStringCopy(ctx->m_context, THIS->m_value, exception);
        if (!exception) JSStringRelease(s);
    });
}
MaybeLocal<String> Value::ToDetailString(Local<Context> context) const { return ToString(context); } // FIXME
MaybeLocal<Object> Value::ToObject(Local<Context> context) const
{
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleLocalException<Object>(this, [&](JSValueRef *exception) {
        JSObjectRef o = JSValueToObject(ctx->m_context, THIS->m_value, exception);
        if (!exception) {
            THIS->m_value = o;
        }
    });
}
MaybeLocal<Integer> Value::ToInteger(Local<Context> context) const
{
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleLocalException<Integer>(this, [&](JSValueRef *exception) {
        JSValueToNumber(ctx->m_context, THIS->m_value, exception);
    });
}
MaybeLocal<Uint32> Value::ToUint32(Local<Context> context) const
{
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleLocalException<Uint32>(this, [&](JSValueRef *exception) {
        JSValueToNumber(ctx->m_context, THIS->m_value, exception);
    });
}
MaybeLocal<Int32> Value::ToInt32(Local<Context> context) const
{
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    return handleLocalException<Int32>(this, [&](JSValueRef *exception) {
        JSValueToNumber(ctx->m_context, THIS->m_value, exception);
    });
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
    
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(isolate);
    if (!impl->m_external_context) {
        impl->m_external_context = static_cast<ContextImpl*>(*Context::New(isolate));
    }
    ContextImpl *ctx = static_cast<ContextImpl*>(impl->m_external_context);
    JSObjectRef external = JSObjectMake(ctx->m_context, s_externalClass, value);
    auto e = ValueImpl::New(ctx, external);
    
    return Local<External>(e.As<External>());
}

void* External::Value() const
{
    return JSObjectGetPrivate((JSObjectRef)reinterpret_cast<const ValueImpl*>(this)->m_value);
}
