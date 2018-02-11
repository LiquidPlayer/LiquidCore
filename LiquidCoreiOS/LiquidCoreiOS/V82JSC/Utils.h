//
//  Utils.hpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Utils_hpp
#define Utils_hpp

#include "Value.h"
#include "Script.h"
#include "Object.h"
#include "TypedArray.h"

class v8::Utils {
public:
    static Local<v8::Value> NewValue(ValueImpl *);
    static Local<v8::Script> NewScript(ScriptImpl *);
    static Local<v8::Object> NewObject(ObjectImpl *);

    static Local<v8::Uint8ClampedArray> NewUint8ClampedArray(Uint8ClampedArrayImpl *impl) { return Local<Uint8ClampedArrayImpl>(impl); }
    static Local<v8::Uint8Array> NewUint8Array(Uint8ArrayImpl *impl) { return Local<Uint8ArrayImpl>(impl); }
    static Local<v8::Uint16Array> NewUint16Array(Uint16ArrayImpl *impl) { return Local<Uint16ArrayImpl>(impl); }
    static Local<v8::Uint32Array> NewUint32Array(Uint32ArrayImpl *impl) { return Local<Uint32ArrayImpl>(impl); }
    static Local<v8::Int8Array> NewInt8Array(Int8ArrayImpl *impl) { return Local<Int8ArrayImpl>(impl); }
    static Local<v8::Int16Array> NewInt16Array(Int16ArrayImpl *impl) { return Local<Int16ArrayImpl>(impl); }
    static Local<v8::Int32Array> NewInt32Array(Int32ArrayImpl *impl) { return Local<Int32ArrayImpl>(impl); }
    static Local<v8::Float32Array> NewFloat32Array(Float32ArrayImpl *impl) { return Local<Float32ArrayImpl>(impl); }
    static Local<v8::Float64Array> NewFloat64Array(Float64ArrayImpl *impl) { return Local<Float64ArrayImpl>(impl); }
};

#endif /* Utils_hpp */
