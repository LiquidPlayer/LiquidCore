/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSObjectRefPrivate.h"
#include "Value.h"
#include "StringImpl.h"

using namespace V82JSC;
using v8::Local;
using v8::EscapableHandleScope;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Isolate;

inline bool is__(const v8::Value* thiz, const char *code_)
{
    v8::HandleScope scope(v8::Isolate::GetCurrent());
    v8::Local<v8::Context> context = ToCurrentContext(thiz);
    auto ctx = ToContextRef(context);
    auto v = ToJSValueRef<v8::Value>(thiz, context);
    
    JSValueRef b = exec(ctx, code_, 1, &v);
    bool ret = JSValueToBoolean(ctx, b);
    return ret;
}
#define IS(name_,code_) bool v8::Value::name_() const { return is__(this, code_); }

Local<v8::Value> Value::New(const Context *ctx, JSValueRef value, BaseMap *map)
{
    JSType t = JSValueGetType(ctx->m_ctxRef, value);
    IsolateImpl* isolateimpl = ToIsolateImpl(ctx);
    v8::Isolate *isolate = ToIsolate(isolateimpl);
    typedef v8::internal::Heap::RootListIndex R;
    v8::internal::Object * io;
    void* resource = nullptr;
    EscapableHandleScope scope(isolate);
    
    double num = 0.0;
    if (!map) {
        switch (t) {
            case kJSTypeUndefined: {
                io = isolateimpl->ii.heap()->root(R::kUndefinedValueRootIndex);
                return scope.Escape(CreateLocal<v8::Value>(isolate, FromHeapPointer(io)));
            }
            case kJSTypeNull: {
                io = isolateimpl->ii.heap()->root(R::kNullValueRootIndex);
                return scope.Escape(CreateLocal<v8::Value>(isolate, FromHeapPointer(io)));
            }
            case kJSTypeBoolean: {
                if (JSValueToBoolean(ctx->m_ctxRef, value))
                    io = isolateimpl->ii.heap()->root(R::kTrueValueRootIndex);
                else
                    io = isolateimpl->ii.heap()->root(R::kFalseValueRootIndex);
                return scope.Escape(CreateLocal<v8::Value>(isolate, FromHeapPointer(io)));
            }
            case kJSTypeString: {
                if (isolateimpl->m_external_strings.count(value) == 1) {
                    Local<v8::WeakExternalString> wes = isolateimpl->m_external_strings[value].Get(isolate);
                    auto ext = ToImpl<WeakExternalString>(wes);
                    map = reinterpret_cast<BaseMap*>(FromHeapPointer(ext->m_map));
                    resource = ext->m_resource;
                } else {
                    map = isolateimpl->m_string_map;
                }
                break;
            }
            case kJSTypeNumber: {
                num = JSValueToNumber(ctx->m_ctxRef, value, 0);
                double intpart;
                if (value != isolateimpl->m_negative_zero && modf(num, &intpart) == 0.0) {
                    if (v8::internal::Smi::IsValid(intpart)) {
                        return scope.Escape(CreateLocalSmi<v8::Value>(reinterpret_cast<v8::internal::Isolate*>(isolateimpl),
                                                                              v8::internal::Smi::FromInt(intpart)));
                    }
                }
                map = isolateimpl->m_number_map;
                break;
            }
            case kJSTypeObject: {
                if (isolateimpl->m_jsobjects.count((JSObjectRef)value)) {
                    auto obj = isolateimpl->m_jsobjects[(JSObjectRef)value];
                    return scope.Escape(CreateLocal<v8::Value>(isolate, obj));
                }
                JSValueRef proxyless = JSObjectGetProxyTarget((JSObjectRef)value);
                if (proxyless == 0) proxyless = value;
                JSValueRef exception = 0;
                JSValueRef isArrayBuffer = exec(ctx->m_ctxRef, "return _1 instanceof ArrayBuffer", 1, &proxyless, &exception);
                if (!exception && JSValueToBoolean(ctx->m_ctxRef, isArrayBuffer)) {
                    map = isolateimpl->m_array_buffer_map;
                } else {
                    exception = 0;
                    JSValueRef isSymbol = exec(ctx->m_ctxRef, "return typeof _1 === 'symbol'", 1, &proxyless, &exception);
                    if (!exception && JSValueToBoolean(ctx->m_ctxRef, isSymbol)) {
                        map = isolateimpl->m_symbol_map;
                    } else {
                        map = isolateimpl->m_value_map;
                    }
                }
                break;
            }
            case kJSTypeSymbol: {
                map = isolateimpl->m_symbol_map;
                break;
            }
            default:
                break;
        }
    }
    
    assert(map);
    
    auto impl = static_cast<Value *>(HeapAllocator::Alloc(isolateimpl, map));
    impl->m_value = value;
    if (t == kJSTypeString) {
        * reinterpret_cast<void**>(reinterpret_cast<intptr_t>(impl) +
                                   v8::internal::Internals::kStringResourceOffset) = resource;
    }
    JSValueProtect(ctx->m_ctxRef, impl->m_value);
    if (t == kJSTypeNumber) {
        reinterpret_cast<v8::internal::HeapNumber*>(ToHeapPointer(impl))->set_value(num);
    }
    if (t == kJSTypeObject) {
        isolateimpl->m_jsobjects[(JSObjectRef)value] = impl;
    }

    return scope.Escape(CreateLocal<v8::Value>(ToIsolate(isolateimpl), impl));
}

void Value::RemoveObjectFromMap(IsolateImpl* iso, JSObjectRef o)
{
    iso->m_jsobjects.erase(o);
}


#define FROMTHIS(c,v) \
    auto cc = ToCurrentContext(this); \
    auto c = ToContextImpl(cc); \
    auto v = ToJSValueRef<Value>(this, cc)

/**
 * Returns true if this value is true.
 */
bool v8::Value::IsTrue() const { FROMTHIS(c,v); return JSValueIsStrictEqual(c->m_ctxRef, v, JSValueMakeBoolean(c->m_ctxRef, true)); }

/**
 * Returns true if this value is false.
 */
bool v8::Value::IsFalse() const { FROMTHIS(c,v); return JSValueIsStrictEqual(c->m_ctxRef, v, JSValueMakeBoolean(c->m_ctxRef, false)); }

/**
 * Returns true if this value is a symbol or a string.
 */
bool v8::Value::IsName() const { return IsString() || IsSymbol(); }

/**
 * Returns true if this value is a symbol.
 */
IS(IsSymbol, "return typeof _1 === 'symbol'")

/**
 * Returns true if this value is a function.
 */
IS(IsFunction, "return typeof _1 === 'function'")

/**
 * Returns true if this value is an array. Note that it will return false for
 * an Proxy for an array.
 */
bool v8::Value::IsArray() const { FROMTHIS(c,v); return JSValueIsArray(c->m_ctxRef, v); }

/**
 * Returns true if this value is an object.
 */
bool v8::Value::IsObject() const { FROMTHIS(c,v); return JSValueIsObject(c->m_ctxRef, v); }

/**
 * Returns true if this value is boolean.
 */
bool v8::Value::IsBoolean() const { FROMTHIS(c,v); return JSValueIsBoolean(c->m_ctxRef, v); }

/**
 * Returns true if this value is a number.
 */
bool v8::Value::IsNumber() const { FROMTHIS(c,v); return JSValueIsNumber(c->m_ctxRef, v); }

/**
 * Returns true if this value is external.
 */
IS(IsExternal, "return Object.prototype.toString.call( _1 ) === '[object External]';")

/**
 * Returns true if this value is a 32-bit signed integer.
 */
bool v8::Value::IsInt32() const
{
    HandleScope scope(Isolate::GetCurrent());
    
    MaybeLocal<Number> num = ToNumber();
    if (!num.IsEmpty()) {
        FROMTHIS(c,v);
        JSValueRef exception = nullptr;
        double number = JSValueToNumber(c->m_ctxRef, v, &exception);
        double intpart;
        if (v == ToIsolateImpl(c)->m_negative_zero) return false;
        if (std::modf(number, &intpart) == 0.0) {
            return (intpart >= std::numeric_limits<std::int32_t>::min() && intpart <= std::numeric_limits<std::int32_t>::max());
        }
    }
    return false;
}

/**
 * Returns true if this value is a 32-bit unsigned integer.
 */
bool v8::Value::IsUint32() const
{
    HandleScope scope(Isolate::GetCurrent());

    MaybeLocal<Number> num = ToNumber();
    if (!num.IsEmpty()) {
        JSValueRef exception = nullptr;
        FROMTHIS(c,v);
        double number = JSValueToNumber(c->m_ctxRef, v, &exception);
        double intpart;
        if (v == ToIsolateImpl(c)->m_negative_zero) return false;
        if (std::modf(number, &intpart) == 0.0) {
            return (intpart >= 0 && intpart <= std::numeric_limits<std::uint32_t>::max());
        }
    }
    return false;
}

/**
 * Returns true if this value is a Date.
 */
bool v8::Value::IsDate() const { FROMTHIS(c,v); return JSValueIsDate(c->m_ctxRef, v); }

/**
 * Returns true if this value is an Arguments object.
 */
IS(IsArgumentsObject, "return Object.prototype.toString.call( _1 ) === '[object Arguments]';")

/**
 * Returns true if this value is a Boolean object.
 */
IS(IsBooleanObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'boolean');")

/**
 * Returns true if this value is a Number object.
 */
IS(IsNumberObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'number');")

/**
 * Returns true if this value is a String object.
 */
IS(IsStringObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'string');")

/**
 * Returns true if this value is a Symbol object.
 */
IS(IsSymbolObject, "return (typeof _1 === 'object' && _1 !== null && typeof _1.valueOf() === 'symbol');")

/**
 * Returns true if this value is a NativeError.
 */
IS(IsNativeError, "return _1 instanceof Error")

/**
 * Returns true if this value is a RegExp.
 */
IS(IsRegExp, "return Object.prototype.toString.call( _1 ) === '[object RegExp]';")

/**
 * Returns true if this value is an async function.
 */
IS(IsAsyncFunction, "return _1 && _1.constructor && _1.constructor.name === 'AsyncFunction';")

/**
 * Returns true if this value is a Generator function.
 */
IS(IsGeneratorFunction, "var Generator = (function*(){}).constructor; return _1 instanceof Generator")

/**
 * Returns true if this value is a Generator object (iterator).
 */
IS(IsGeneratorObject, "return _1 && typeof _1[Symbol.iterator] === 'function'")

/**
 * Returns true if this value is a Promise.
 */
IS(IsPromise, "return _1 && Promise && Promise.resolve && Promise.resolve(_1) == _1")

/**
 * Returns true if this value is a Map.
 */
IS(IsMap, "return _1 instanceof Map")

/**
 * Returns true if this value is a Set.
 */
IS(IsSet, "return _1 instanceof Set")

/**
 * Returns true if this value is a Map Iterator.
 */
bool v8::Value::IsMapIterator() const { return false; } // FIXME

/**
 * Returns true if this value is a Set Iterator.
 */
bool v8::Value::IsSetIterator() const { return false; } // FIXME

/**
 * Returns true if this value is a WeakMap.
 */
IS(IsWeakMap, "return _1 instanceof WeakMap")

/**
 * Returns true if this value is a WeakSet.
 */
IS(IsWeakSet, "return _1 instanceof WeakSet")

/**
 * Returns true if this value is an ArrayBuffer.
 */
IS(IsArrayBuffer, "return _1 instanceof ArrayBuffer")

/**
 * Returns true if this value is an ArrayBufferView.
 */
IS(IsArrayBufferView, "return _1 && _1.buffer instanceof ArrayBuffer && _1.byteLength !== undefined")

/**
 * Returns true if this value is one of TypedArrays.
 */
IS(IsTypedArray, "return _1 && ArrayBuffer.isView(_1) && Object.prototype.toString.call(_1) !== '[object DataView]'")

/**
 * Returns true if this value is an Uint8Array.
 */
IS(IsUint8Array, "return _1 instanceof Uint8Array")

/**
 * Returns true if this value is an Uint8ClampedArray.
 */
IS(IsUint8ClampedArray, "return _1 instanceof Uint8ClampedArray")

/**
 * Returns true if this value is an Int8Array.
 */
IS(IsInt8Array, "return _1 instanceof Int8Array")

/**
 * Returns true if this value is an Uint16Array.
 */
IS(IsUint16Array, "return _1 instanceof Uint16Array")

/**
 * Returns true if this value is an Int16Array.
 */
IS(IsInt16Array, "return _1 instanceof Int16Array")

/**
 * Returns true if this value is an Uint32Array.
 */
IS(IsUint32Array, "return _1 instanceof Uint32Array")

/**
 * Returns true if this value is an Int32Array.
 */
IS(IsInt32Array, "return _1 instanceof Int32Array")

/**
 * Returns true if this value is a Float32Array.
 */
IS(IsFloat32Array, "return _1 instanceof Float32Array")

/**
 * Returns true if this value is a Float64Array.
 */
IS(IsFloat64Array, "return _1 instanceof Float64Array")

/**
 * Returns true if this value is a DataView.
 */
IS(IsDataView, "return _1 && Object.prototype.toString.call(_1) === '[object DataView]'")

/**
 * Returns true if this value is a SharedArrayBuffer.
 * This is an experimental feature.
 */
bool v8::Value::IsSharedArrayBuffer() const { return false; } // FIXME

/**
 * Returns true if this value is a JavaScript Proxy.
 */
bool v8::Value::IsProxy() const
{
    Isolate *isolate = ToIsolate(this);
    HandleScope scope(isolate);
    Local<v8::Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef maybe_proxy = ToJSValueRef(this, context);
    
    return (JSValueIsObject(ctx, maybe_proxy) && JSObjectGetProxyTarget((JSObjectRef)maybe_proxy) != 0);
}

bool v8::Value::IsWebAssemblyCompiledModule() const { return false; } // FIXME

template <typename T, typename F>
Maybe<T> handleException(IsolateImpl* isolate, F&& lambda)
{
    LocalException exception(isolate);
    T value = lambda(&exception);
    if (!exception.ShouldThrow()) {
        return _maybe<T>(value).toMaybe();
    }
    return v8::Nothing<T>();
}

template <typename T>
Maybe<T> toValue(const v8::Value* thiz, Local<v8::Context> context, bool isNaNZero=false)
{
    auto ctx = ToContextImpl(context);
    JSValueRef value = ToJSValueRef<v8::Value>(thiz, context);
    IsolateImpl* i = ToIsolateImpl(ctx);

    LocalException exception(i);
    T ret;
    if (std::is_same<T,bool>::value) {
        ret = JSValueToBoolean(ctx->m_ctxRef, value);
    } else {
        double number = JSValueToNumber(ctx->m_ctxRef, value, &exception);
        if (isNaNZero && std::isnan(number)) number = 0;
        ret = static_cast<T>(number);
    }
    if (!exception.ShouldThrow()) {
        return _maybe<T>(ret).toMaybe();
    }
    return v8::Nothing<T>();
}
Maybe<bool> v8::Value::BooleanValue(Local<v8::Context> context) const    { return toValue<bool>(this, context); }
Maybe<double> v8::Value::NumberValue(Local<v8::Context> context) const   { return toValue<double>(this, context); }
Maybe<int64_t> v8::Value::IntegerValue(Local<v8::Context> context) const { return toValue<int64_t>(this, context, true); }
Maybe<uint32_t> v8::Value::Uint32Value(Local<v8::Context> context) const { return toValue<uint32_t>(this, context, true); }
Maybe<int32_t> v8::Value::Int32Value(Local<v8::Context> context) const   { return toValue<int32_t>(this, context, true); }

Maybe<bool> v8::Value::Equals(Local<v8::Context> context, Local<v8::Value> that) const
{
    JSValueRef this_ = ToJSValueRef<v8::Value>(this, context);
    JSValueRef that_ = ToJSValueRef<v8::Value>(that, context);
    JSContextRef context_ = ToContextRef(context);
    IsolateImpl* i = ToIsolateImpl(ToContextImpl(context));

    LocalException exception(i);
    bool is = JSValueIsEqual(context_, this_, that_, &exception);
    if (!exception.ShouldThrow()) {
        return _maybe<bool>(is).toMaybe();
    }
    return Nothing<bool>();
}
bool v8::Value::StrictEquals(Local<v8::Value> that) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<v8::Context> context = ToCurrentContext(this);
    JSValueRef this_ = ToJSValueRef(this, context);
    JSValueRef that_ = ToJSValueRef(that, context);
    return JSValueIsStrictEqual(ToContextRef(context), this_, that_);
}
bool v8::Value::SameValue(Local<v8::Value> that) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<v8::Context> context = ToCurrentContext(this);
    JSValueRef this_ = ToJSValueRef(this, context);
    JSValueRef that_ = ToJSValueRef(that, context);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));

    if (this_ == that_) return true;
    if (this_ == iso->m_negative_zero || that_ == iso->m_negative_zero) {
        return false;
    }
    return JSValueIsStrictEqual(ToContextRef(context), this_, that_);
}

Local<v8::String> v8::Value::TypeOf(Isolate* isolate)
{
    FROMTHIS(c,v);
    JSValueRef exception = nullptr;
    JSValueRef to = exec(c->m_ctxRef, "return typeof _1", 1, &v);
    return V82JSC::String::New(isolate, JSValueToStringCopy(c->m_ctxRef, to, &exception));
}

Maybe<bool> v8::Value::InstanceOf(Local<v8::Context> context, Local<Object> object)
{
    FROMTHIS(c,v);
    JSValueRef args[] = {
        v,
        ToJSValueRef(object, context)
    };
    LocalException exception(ToIsolateImpl(ToContextImpl(context)));
    JSValueRef is = exec(c->m_ctxRef, "return _1 instanceof _2", 2, args, &exception);
    if (exception.ShouldThrow()) {
        return Nothing<bool>();
    }
    return _maybe<bool>(JSValueToBoolean(c->m_ctxRef, is)).toMaybe();
}

MaybeLocal<v8::Uint32> v8::Value::ToArrayIndex(Local<v8::Context> context) const
{
    HandleScope scope(Isolate::GetCurrent());

    MaybeLocal<Number> num = ToNumber(context);
    if (!num.IsEmpty() && num.ToLocalChecked()->IsUint32()) {
        return ToUint32(context);
    }
    return MaybeLocal<Uint32>();
}

MaybeLocal<v8::Boolean> v8::Value::ToBoolean(Local<v8::Context> context) const
{
    auto ctx = ToContextImpl(context);
    JSValueRef v = ToJSValueRef(this, context);
    return V82JSC::Value::New(ctx, JSValueMakeBoolean(ctx->m_ctxRef,
              JSValueToBoolean(ctx->m_ctxRef, v))).As<v8::Boolean>();
}
MaybeLocal<v8::String> v8::Value::ToString(Local<v8::Context> context) const
{
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    EscapableHandleScope scope(ToIsolate(iso));

    JSValueRef v = ToJSValueRef(this, context);
    LocalException exception(iso);
    JSStringRef s = JSValueToStringCopy (ctx->m_ctxRef, v, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<String>();
    }
    return scope.Escape(V82JSC::String::New(reinterpret_cast<Isolate*>(iso), s));
}
MaybeLocal<v8::String> v8::Value::ToDetailString(Local<v8::Context> context) const { return ToString(context); } // FIXME
MaybeLocal<v8::Object> v8::Value::ToObject(Local<v8::Context> context) const
{
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    EscapableHandleScope scope(ToIsolate(iso));

    JSValueRef v = ToJSValueRef(this, context);
    LocalException exception(iso);
    JSObjectRef o = JSValueToObject(ctx->m_ctxRef, v, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<Object>();
    }
    return scope.Escape(V82JSC::Value::New(ctx, o).As<Object>());
}

template<class T, typename C>
MaybeLocal<T> ToNum(const v8::Value* thiz, Local<v8::Context> context)
{
    auto ctx = ToContextImpl(context);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    JSValueRef v = ToJSValueRef(thiz, context);
    LocalException exception(iso);
    double num = JSValueToNumber(ctx->m_ctxRef, v, &exception);
    if (exception.ShouldThrow()) {
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
    Local<v8::Number> nval = v8::Number::New(reinterpret_cast<Isolate*>(iso), val);
    return nval.As<T>();
}

MaybeLocal<v8::Number> v8::Value::ToNumber(Local<v8::Context> context) const
{
    return ToNum<Number, double>(this, context);
}
MaybeLocal<v8::Integer> v8::Value::ToInteger(Local<v8::Context> context) const
{
    return ToNum<Integer, int32_t>(this, context);
}
MaybeLocal<v8::Uint32> v8::Value::ToUint32(Local<v8::Context> context) const
{
    return ToNum<v8::Uint32, uint32_t>(this, context);
}
MaybeLocal<v8::Int32> v8::Value::ToInt32(Local<v8::Context> context) const
{
    return ToNum<Int32, int32_t>(this, context);
}

JSClassRef s_externalClass = nullptr;

Local<v8::External> v8::External::New(Isolate* isolate, void* value)
{
    EscapableHandleScope scope(isolate);
    if (!s_externalClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.attributes |= kJSClassAttributeNoAutomaticPrototype;
        definition.className = "External";
        s_externalClass = JSClassCreate(&definition);
    }
    
    Local<v8::Context> context = ToIsolateImpl(isolate)->m_nullContext.Get(isolate);
    JSObjectRef external = JSObjectMake(ToContextRef(context), s_externalClass, value);
    auto e = V82JSC::Value::New(ToContextImpl(context), external);
    
    return scope.Escape(e.As<External>());
}

void* v8::External::Value() const
{
    auto context = ToCurrentContext(this);
    auto v = ToJSValueRef<External>(this, context);
    return JSObjectGetPrivate((JSObjectRef)v);
}
