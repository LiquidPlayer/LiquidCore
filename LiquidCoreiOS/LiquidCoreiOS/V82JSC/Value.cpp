//
//  Value.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Value.h"
#include "Utils.h"

using namespace v8;

Local<Value> ValueImpl::New(ContextImpl *ctx, JSValueRef value)
{
    ValueImpl * impl = (ValueImpl *) malloc(sizeof(ValueImpl));
    memset(impl, 0, sizeof(ValueImpl));
    
    impl->m_context = ctx;
    impl->m_value = value;
    
    Local<Value> v;
    *(reinterpret_cast<Value**>(&v)) = impl;
    return v;
}


/**
 * Returns true if this value is true.
 */
bool Value::IsTrue() const { return false; }

/**
 * Returns true if this value is false.
 */
bool Value::IsFalse() const { return false; }

/**
 * Returns true if this value is a symbol or a string.
 */
bool Value::IsName() const { return false; }

/**
 * Returns true if this value is a symbol.
 */
bool Value::IsSymbol() const { return false; }

/**
 * Returns true if this value is a function.
 */
bool Value::IsFunction() const { return false; }

/**
 * Returns true if this value is an array. Note that it will return false for
 * an Proxy for an array.
 */
bool Value::IsArray() const { return false; }

/**
 * Returns true if this value is an object.
 */
bool Value::IsObject() const { return false; }

/**
 * Returns true if this value is boolean.
 */
bool Value::IsBoolean() const { return false; }

/**
 * Returns true if this value is a number.
 */
bool Value::IsNumber() const { return false; }

/**
 * Returns true if this value is external.
 */
bool Value::IsExternal() const { return false; }

/**
 * Returns true if this value is a 32-bit signed integer.
 */
bool Value::IsInt32() const { return false; }

/**
 * Returns true if this value is a 32-bit unsigned integer.
 */
bool Value::IsUint32() const { return false; }

/**
 * Returns true if this value is a Date.
 */
bool Value::IsDate() const { return false; }

/**
 * Returns true if this value is an Arguments object.
 */
bool Value::IsArgumentsObject() const { return false; }

/**
 * Returns true if this value is a Boolean object.
 */
bool Value::IsBooleanObject() const { return false; }

/**
 * Returns true if this value is a Number object.
 */
bool Value::IsNumberObject() const { return false; }

/**
 * Returns true if this value is a String object.
 */
bool Value::IsStringObject() const { return false; }

/**
 * Returns true if this value is a Symbol object.
 */
bool Value::IsSymbolObject() const { return false; }

/**
 * Returns true if this value is a NativeError.
 */
bool Value::IsNativeError() const { return false; }

/**
 * Returns true if this value is a RegExp.
 */
bool Value::IsRegExp() const { return false; }

/**
 * Returns true if this value is an async function.
 */
bool Value::IsAsyncFunction() const { return false; }

/**
 * Returns true if this value is a Generator function.
 */
bool Value::IsGeneratorFunction() const { return false; }

/**
 * Returns true if this value is a Generator object (iterator).
 */
bool Value::IsGeneratorObject() const { return false; }

/**
 * Returns true if this value is a Promise.
 */
bool Value::IsPromise() const { return false; }

/**
 * Returns true if this value is a Map.
 */
bool Value::IsMap() const { return false; }

/**
 * Returns true if this value is a Set.
 */
bool Value::IsSet() const { return false; }

/**
 * Returns true if this value is a Map Iterator.
 */
bool Value::IsMapIterator() const { return false; }

/**
 * Returns true if this value is a Set Iterator.
 */
bool Value::IsSetIterator() const { return false; }

/**
 * Returns true if this value is a WeakMap.
 */
bool Value::IsWeakMap() const { return false; }

/**
 * Returns true if this value is a WeakSet.
 */
bool Value::IsWeakSet() const { return false; }

/**
 * Returns true if this value is an ArrayBuffer.
 */
bool Value::IsArrayBuffer() const { return false; }

/**
 * Returns true if this value is an ArrayBufferView.
 */
bool Value::IsArrayBufferView() const { return false; }

/**
 * Returns true if this value is one of TypedArrays.
 */
bool Value::IsTypedArray() const { return false; }

/**
 * Returns true if this value is an Uint8Array.
 */
bool Value::IsUint8Array() const { return false; }

/**
 * Returns true if this value is an Uint8ClampedArray.
 */
bool Value::IsUint8ClampedArray() const { return false; }

/**
 * Returns true if this value is an Int8Array.
 */
bool Value::IsInt8Array() const { return false; }

/**
 * Returns true if this value is an Uint16Array.
 */
bool Value::IsUint16Array() const { return false; }

/**
 * Returns true if this value is an Int16Array.
 */
bool Value::IsInt16Array() const { return false; }

/**
 * Returns true if this value is an Uint32Array.
 */
bool Value::IsUint32Array() const { return false; }

/**
 * Returns true if this value is an Int32Array.
 */
bool Value::IsInt32Array() const { return false; }

/**
 * Returns true if this value is a Float32Array.
 */
bool Value::IsFloat32Array() const { return false; }

/**
 * Returns true if this value is a Float64Array.
 */
bool Value::IsFloat64Array() const { return false; }

/**
 * Returns true if this value is a DataView.
 */
bool Value::IsDataView() const { return false; }

/**
 * Returns true if this value is a SharedArrayBuffer.
 * This is an experimental feature.
 */
bool Value::IsSharedArrayBuffer() const { return false; }

/**
 * Returns true if this value is a JavaScript Proxy.
 */
bool Value::IsProxy() const { return false; }

bool Value::IsWebAssemblyCompiledModule() const { return false; }

template <class T>
class _maybe {
public:
    bool has_value_;
    T value_;
};

Maybe<bool> Value::BooleanValue(Local<Context> context) const { return Nothing<bool>(); }
Maybe<double> Value::NumberValue(Local<Context> context) const { return Nothing<double>(); }
Maybe<int64_t> Value::IntegerValue(Local<Context> context) const { return Nothing<int64_t>(); }
Maybe<uint32_t> Value::Uint32Value(Local<Context> context) const { return Nothing<uint32_t>(); }
Maybe<int32_t> Value::Int32Value(Local<Context> context) const {
    ValueImpl *impl = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(this));
    JSValueRef exception;
    ContextImpl *ctx = static_cast<ContextImpl *>(*context);
    double value = JSValueToNumber(ctx->m_context, impl->m_value, &exception);
    if (!exception) {
        _maybe<int32_t> maybe;
        maybe.has_value_ = true;
        maybe.value_ = (uint32_t)value;
        return *(reinterpret_cast<Maybe<int32_t> *>(&maybe));
    }
    return Nothing<int32_t>();
}
Maybe<bool> Value::Equals(Local<Context> context, Local<Value> that) const { return Nothing<bool>(); }
bool Value::StrictEquals(Local<Value> that) const { return false; }
bool Value::SameValue(Local<Value> that) const { return false; }

Local<String> Value::TypeOf(Isolate*) { return Local<String>(); }

Maybe<bool> Value::InstanceOf(Local<Context> context, Local<Object> object) { return Nothing<bool>(); }

MaybeLocal<Uint32> Value::ToArrayIndex(Local<Context> context) const { return MaybeLocal<Uint32>(); }


MaybeLocal<v8::Boolean> Value::ToBoolean(Local<Context> context) const { return MaybeLocal<v8::Boolean>(); }
MaybeLocal<Number> Value::ToNumber(Local<Context> context) const { return MaybeLocal<Number>(); }
MaybeLocal<String> Value::ToString(Local<Context> context) const { return MaybeLocal<String>(); }
MaybeLocal<String> Value::ToDetailString(Local<Context> context) const { return MaybeLocal<String>(); }
MaybeLocal<Object> Value::ToObject(Local<Context> context) const { return MaybeLocal<Object>(); }
MaybeLocal<Integer> Value::ToInteger(Local<Context> context) const { return MaybeLocal<Integer>(); }
MaybeLocal<Uint32> Value::ToUint32(Local<Context> context) const { return MaybeLocal<Uint32>(); }
MaybeLocal<Int32> Value::ToInt32(Local<Context> context) const { return MaybeLocal<Int32>(); }


Local<External> External::New(Isolate* isolate, void* value)
{
    return Local<External>();
}
void* External::Value() const
{
    return nullptr;
}
