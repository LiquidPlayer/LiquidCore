/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;

size_t v8::TypedArray::Length()
{
    HandleScope scope(ToIsolate(this));
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) ToJSValueRef(this, context);
    
    JSValueRef excp=0;
    size_t length = JSObjectGetTypedArrayLength(ctx, obj, &excp);
    assert(excp==0);
    return length;
}

template <typename T>
v8::Local<T> NewTypedArray(JSTypedArrayType arrayType, v8::Local<v8::ArrayBuffer> array_buffer, size_t byte_offset, size_t byte_length)
{
    v8::EscapableHandleScope scope(ToIsolate(*array_buffer));
    v8::Local<v8::Context> context = ToCurrentContext(*array_buffer);
    JSContextRef ctx = ToContextRef(context);

    auto impl = ToImpl<Value>(array_buffer);
    JSValueRef excp=0;
    JSObjectRef typed_array = JSObjectMakeTypedArrayWithArrayBufferAndOffset(ctx, arrayType, (JSObjectRef) impl->m_value, byte_offset, byte_length, &excp);
    assert(excp==0);
    v8::Local<T> array = Value::New(ToContextImpl(context), typed_array).As<T>();
    return scope.Escape(array);
}
template <typename T>
v8::Local<T> NewSharedTypedArray(JSTypedArrayType arrayType, v8::Local<v8::SharedArrayBuffer> shared_array_buffer,
                size_t byte_offset, size_t byte_length)
{
    NOT_IMPLEMENTED;
}

#define TYPEDARRAY_CONSTRUCTORS(T,type) \
v8::Local<T> T::New(v8::Local<v8::ArrayBuffer> array_buffer, size_t byte_offset, size_t length) \
{ \
    return NewTypedArray<T>(type, array_buffer, byte_offset, length); \
} \
v8::Local<T> T::New(v8::Local<v8::SharedArrayBuffer> shared_array_buffer, size_t byte_offset, size_t length) \
{ \
    return NewSharedTypedArray<T>(type, shared_array_buffer, byte_offset, length); \
}

TYPEDARRAY_CONSTRUCTORS(v8::Uint8ClampedArray, kJSTypedArrayTypeUint8ClampedArray)
TYPEDARRAY_CONSTRUCTORS(v8::Uint8Array, kJSTypedArrayTypeUint8Array)
TYPEDARRAY_CONSTRUCTORS(v8::Uint16Array, kJSTypedArrayTypeUint16Array)
TYPEDARRAY_CONSTRUCTORS(v8::Uint32Array, kJSTypedArrayTypeUint32Array)
TYPEDARRAY_CONSTRUCTORS(v8::Int8Array, kJSTypedArrayTypeInt8Array)
TYPEDARRAY_CONSTRUCTORS(v8::Int16Array, kJSTypedArrayTypeInt16Array)
TYPEDARRAY_CONSTRUCTORS(v8::Int32Array, kJSTypedArrayTypeInt32Array)
TYPEDARRAY_CONSTRUCTORS(v8::Float32Array, kJSTypedArrayTypeFloat32Array)
TYPEDARRAY_CONSTRUCTORS(v8::Float64Array, kJSTypedArrayTypeFloat64Array)

// FIXME
TYPEDARRAY_CONSTRUCTORS(v8::BigUint64Array, kJSTypedArrayTypeFloat64Array)
TYPEDARRAY_CONSTRUCTORS(v8::BigInt64Array, kJSTypedArrayTypeFloat64Array)

