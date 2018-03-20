//
//  V82JSC.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 3/18/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef V82JSC_h
#define V82JSC_h

#include <JavaScriptCore/JavaScript.h>
#include "include/v8-util.h"
#include "src/arguments.h"
#include "src/base/platform/platform.h"
#include "src/code-stubs.h"
#include "src/compilation-cache.h"
#include "src/debug/debug.h"
#include "src/execution.h"
#include "src/futex-emulation.h"
#include "src/heap/incremental-marking.h"
#include "src/api.h"
#include "src/lookup.h"
#include "src/objects-inl.h"
#include "src/parsing/preparse-data.h"
#include "src/profiler/cpu-profiler.h"
#include "src/unicode-inl.h"
#include "src/utils.h"
#include "src/vm-state.h"
#include "src/heap/heap.h"

#define DEF(T,V,F) \
v8::internal::Object ** V;
struct Roots {
    STRONG_ROOT_LIST(DEF)
};

struct IsolateImpl {
    void *i0; // kHeapObjectMapOffset, kIsolateEmbedderDataOffset
    void *i1; // kForeignAddressOffset
    void *i2;
    void *i3;
    uint64_t i64_0; // kExternalMemoryOffset
    uint64_t i64_1; // kExternalMemoryLimitOffset
    uint64_t i64_2;
    void *i4;
    void *i5;
    struct Roots roots; // kIsolateRootsOffset
    JSContextGroupRef m_group;
    
    void EnterContext(v8::Context *ctx);
    void ExitContext(v8::Context *ctx);
    
    v8::Context *current_context;
    v8::Context *m_external_context;
};

struct ContextImpl;

struct InternalObjectImpl {
    union {
        v8::internal::Map *pMap;
        v8::internal::Oddball oddball;
        v8::internal::Map map;
        unsigned char filler_[256]; // FIXME
    };
    JSValueRef  m_value;
    JSStringRef m_string;
    JSObjectRef m_object;
    ContextImpl* m_context;
};

struct ContextImpl : v8::Context
{
    v8::internal::Context *pInternal;
    
    JSContextRef m_context;
    IsolateImpl *isolate;
    
    typedef enum _IsFunctions {
        IsFunction,
        IsSymbol,
        IsArgumentsObject,
        IsBooleanObject,
        IsNumberObject,
        IsStringObject,
        IsSymbolObject,
        IsNativeError,
        IsRegExp,
        IsAsyncFunction,
        IsGeneratorFunction,
        IsGeneratorObject,
        IsPromise,
        IsMap,
        IsSet,
        IsMapIterator,
        IsSetIterator,
        IsWeakMap,
        IsWeakSet,
        IsArrayBuffer,
        IsArrayBufferView,
        IsTypedArray,
        IsUint8Array,
        IsUint8ClampedArray,
        IsInt8Array,
        IsUint16Array,
        IsInt16Array,
        IsUint32Array,
        IsInt32Array,
        IsFloat32Array,
        IsFloat64Array,
        IsDataView,
        IsSharedArrayBuffer,
        IsProxy,
        IsExternal,
        
        TypeOf,
        InstanceOf,
        
        SIZE
    } IsFunctions;
    JSObjectRef IsFunctionRefs[IsFunctions::SIZE];
};

struct ArrayBufferViewImpl : v8::ArrayBufferView
{
    int m_byte_length;
};

struct ScriptImpl : v8::Script
{
    JSStringRef m_sourceURL;
    int m_startingLineNumber;
    JSStringRef m_script;
};

struct StringImpl : InternalObjectImpl {
    static v8::Local<v8::String> New(JSStringRef string,
                                     v8::internal::InstanceType type=v8::internal::FIRST_NONSTRING_TYPE,
                                     void *resource = nullptr);
};

struct ValueImpl : InternalObjectImpl, v8::Value {    
    static v8::Local<Value> New(ContextImpl *ctx, JSValueRef value);
};

struct ObjectImpl : public ValueImpl, v8::Object {
    JSObjectRef m_object;
    
    static v8::Local<v8::Object> New(ContextImpl *ctx, JSObjectRef object);
};

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

struct UndefinedImpl : InternalObjectImpl {
    static v8::Primitive * New(v8::Isolate *isolate);
};

#endif /* V82JSC_h */
