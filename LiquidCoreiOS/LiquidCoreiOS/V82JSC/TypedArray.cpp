//
//  TypedArray.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

#define TYPEDARRAY_CONSTRUCTORS(T,s) \
Local<T> T::New(Local<ArrayBuffer> array_buffer, \
size_t byte_offset, size_t length) \
{ \
    T##Impl * impl = (T##Impl *) malloc(sizeof(T##Impl)); \
    memset(impl, 0, sizeof(T##Impl)); \
    impl->m_byte_length = s; \
    return Local<T>(); \
} \
Local<T> T::New(Local<SharedArrayBuffer> shared_array_buffer, \
                size_t byte_offset, size_t length) \
{ \
    T##Impl * impl = (T##Impl *) malloc(sizeof(T##Impl)); \
    memset(impl, 0, sizeof(T##Impl)); \
    impl->m_byte_length = s; \
    return Local<T>(); \
}

size_t TypedArray::Length()
{
    return 0;
}

TYPEDARRAY_CONSTRUCTORS(Uint8ClampedArray, 1)
TYPEDARRAY_CONSTRUCTORS(Uint8Array, 1)
TYPEDARRAY_CONSTRUCTORS(Uint16Array, 2)
TYPEDARRAY_CONSTRUCTORS(Uint32Array, 4)
TYPEDARRAY_CONSTRUCTORS(Int8Array, 1)
TYPEDARRAY_CONSTRUCTORS(Int16Array, 2)
TYPEDARRAY_CONSTRUCTORS(Int32Array, 4)
TYPEDARRAY_CONSTRUCTORS(Float32Array, 4)
TYPEDARRAY_CONSTRUCTORS(Float64Array, 8)

