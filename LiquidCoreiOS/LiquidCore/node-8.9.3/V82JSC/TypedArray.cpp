//
//  TypedArray.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "V82JSC.h"

size_t v8::TypedArray::Length()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSObjectRef obj = (JSObjectRef) V82JSC::ToJSValueRef(this, context);
    
    JSValueRef excp=0;
    size_t length = JSObjectGetTypedArrayLength(ctx, obj, &excp);
    assert(excp==0);
    return length;
}

template <typename T>
v8::Local<T> NewTypedArray(JSTypedArrayType arrayType, v8::Local<v8::ArrayBuffer> array_buffer, size_t byte_offset, size_t byte_length)
{
    v8::Local<v8::Context> context = V82JSC::ToCurrentContext(*array_buffer);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(array_buffer);
    JSValueRef excp=0;
    JSObjectRef typed_array = JSObjectMakeTypedArrayWithArrayBufferAndOffset(ctx, arrayType, (JSObjectRef) impl->m_value, byte_offset, byte_length, &excp);
    assert(excp==0);
    v8::Local<T> array = ValueImpl::New(V82JSC::ToContextImpl(context), typed_array).As<T>();
    return array;
}
template <typename T>
v8::Local<T> NewSharedTypedArray(JSTypedArrayType arrayType, v8::Local<v8::SharedArrayBuffer> shared_array_buffer,
                size_t byte_offset, size_t byte_length)
{
    assert(0);
    return v8::Local<T>();
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

