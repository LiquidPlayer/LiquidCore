//
//  TypedArray.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

size_t TypedArray::Length()
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,TypedArray>(this);
    JSValueRef excp=0;
    size_t length = JSObjectGetTypedArrayLength(impl->m_context->m_ctxRef, (JSObjectRef)impl->m_value, &excp);
    assert(excp==0);
    return length;
}

template <typename T>
Local<T> NewTypedArray(JSTypedArrayType arrayType, Local<ArrayBuffer> array_buffer, size_t byte_offset, size_t byte_length)
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(array_buffer);
    JSContextRef ctx = impl->m_context->m_ctxRef;
    JSValueRef excp=0;
    JSObjectRef typed_array = JSObjectMakeTypedArrayWithArrayBufferAndOffset(ctx, arrayType, (JSObjectRef) impl->m_value, byte_offset, byte_length, &excp);
    assert(excp==0);
    Local<T> array = ValueImpl::New(impl->m_context, typed_array).As<T>();
    GetArrayBufferViewInfo(V82JSC::ToImpl<ArrayBufferView>(array));
    return array;
}
template <typename T>
Local<T> NewSharedTypedArray(JSTypedArrayType arrayType, Local<SharedArrayBuffer> shared_array_buffer,
                size_t byte_offset, size_t byte_length)
{
    assert(0);
    return Local<T>();
}

#define TYPEDARRAY_CONSTRUCTORS(T,type) \
Local<T> T::New(Local<ArrayBuffer> array_buffer, size_t byte_offset, size_t length) \
{ \
    return NewTypedArray<T>(type, array_buffer, byte_offset, length); \
} \
Local<T> T::New(Local<SharedArrayBuffer> shared_array_buffer, size_t byte_offset, size_t length) \
{ \
    return NewSharedTypedArray<T>(type, shared_array_buffer, byte_offset, length); \
}

TYPEDARRAY_CONSTRUCTORS(Uint8ClampedArray, kJSTypedArrayTypeUint8ClampedArray)
TYPEDARRAY_CONSTRUCTORS(Uint8Array, kJSTypedArrayTypeUint8Array)
TYPEDARRAY_CONSTRUCTORS(Uint16Array, kJSTypedArrayTypeUint16Array)
TYPEDARRAY_CONSTRUCTORS(Uint32Array, kJSTypedArrayTypeUint32Array)
TYPEDARRAY_CONSTRUCTORS(Int8Array, kJSTypedArrayTypeInt8Array)
TYPEDARRAY_CONSTRUCTORS(Int16Array, kJSTypedArrayTypeInt16Array)
TYPEDARRAY_CONSTRUCTORS(Int32Array, kJSTypedArrayTypeInt32Array)
TYPEDARRAY_CONSTRUCTORS(Float32Array, kJSTypedArrayTypeFloat32Array)
TYPEDARRAY_CONSTRUCTORS(Float64Array, kJSTypedArrayTypeFloat64Array)

