//
//  TypedArray.hpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef TypedArray_h
#define TypedArray_h

#include <v8.h>
#include <JavaScriptCore/JavaScript.h>
#include "TypedArray.h"
#include "ArrayBuffer.h"

struct TypedArrayImpl : public ArrayBufferViewImpl, v8::TypedArray {
};

struct Uint8ClampedArrayImpl : public TypedArrayImpl, v8::Uint8ClampedArray {
};

struct Uint8ArrayImpl : public TypedArrayImpl, v8::Uint8Array {
};

struct Uint16ArrayImpl : public TypedArrayImpl, v8::Uint16Array {
};

struct Uint32ArrayImpl : public TypedArrayImpl, v8::Uint32Array {
};

struct Int8ArrayImpl : public TypedArrayImpl, v8::Int8Array {
};

struct Int16ArrayImpl : public TypedArrayImpl, v8::Int16Array {
};

struct Int32ArrayImpl : public TypedArrayImpl, v8::Int32Array {
};

struct Float32ArrayImpl : public TypedArrayImpl, v8::Float32Array {
};

struct Float64ArrayImpl : public TypedArrayImpl, v8::Float64Array {
};

#endif /* TypedArray_h */
