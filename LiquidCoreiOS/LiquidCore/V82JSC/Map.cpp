/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using v8::Local;
using v8::EscapableHandleScope;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Isolate;
using v8::NativeWeakMap;

Local<v8::NativeWeakMap> v8::NativeWeakMap::New(Isolate* isolate)
{
    Local<Context> context = OperatingContext(isolate);
    
    Local<v8::Value> nwm = V82JSC::Value::New(ToContextImpl(context),
        exec(ToContextRef(context),
        "return new WeakMap()", 0, 0));
    
    Local<NativeWeakMap> loc = * (reinterpret_cast<Local<NativeWeakMap> *>(&nwm));
    return loc;
}
void v8::NativeWeakMap::Set(Local<Value> key, Local<Value> value)
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(this, context),
        ToJSValueRef(key, context),
        ToJSValueRef(value, context)
    };
    exec(ctx, "_1[_2] = _3", 3, args);
}
Local<v8::Value> NativeWeakMap::Get(Local<Value> key) const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(this, context),
        ToJSValueRef(key, context),
    };
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx, "return _1[_2]", 2, args));
}
bool NativeWeakMap::Has(Local<Value> key)
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(this, context),
        ToJSValueRef(key, context),
    };
    return JSValueToBoolean(ctx, exec(ctx, "return Object.getOwnPropertyDescriptor(_1,_2) !== undefined", 2, args));
}
bool NativeWeakMap::Delete(Local<Value> key)
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(this, context),
        ToJSValueRef(key, context),
    };
    return JSValueToBoolean(ctx, exec(ctx, "return delete _1[_2]", 2, args));
}


size_t v8::Map::Size() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);

    JSStringRef ssize = JSStringCreateWithUTF8CString("size");
    JSValueRef excp = 0;
    JSValueRef size = JSObjectGetProperty(ctx, (JSObjectRef)obj, ssize, &excp);
    assert(excp==0);
    return JSValueToNumber(ctx, size, 0);
}

void v8::Map::Clear()
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    exec(ctx, "_1.clear()", 1, &obj);
}

MaybeLocal<v8::Value> v8::Map::Get(Local<Context> context, Local<Value> key)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    LocalException exception(iso);
    JSValueRef args[] = {
        obj,
        ToJSValueRef(key, context)
    };
    JSValueRef r = exec(ctx, "return _1.get(_2)", 2, args, &exception);
    if (exception.ShouldThrow()) return MaybeLocal<Value>();
    return V82JSC::Value::New(ToContextImpl(context), r);
}

MaybeLocal<v8::Map> v8::Map::Set(Local<v8::Context> context,
                                 Local<v8::Value> key,
                                 Local<v8::Value> value)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    LocalException exception(iso);
    auto impl = ToImpl<V82JSC::Value, Map>(this);
    JSValueRef args[] = {
        obj,
        ToJSValueRef(key, context),
        ToJSValueRef(value, context)
    };
    exec(ctx, "_1.set(_2, _3)", 3, args, &exception);
    if (exception.ShouldThrow()) return MaybeLocal<Map>();
    return CreateLocal<v8::Map>(ToIsolate(iso), impl);
}

Maybe<bool> v8::Map::Has(Local<v8::Context> context, Local<v8::Value> key)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    LocalException exception(iso);
    JSValueRef args[] = {
        obj,
        ToJSValueRef(key, context)
    };
    JSValueRef r = exec(ctx, "return _1.has(_2)", 2, args, &exception);
    if (exception.ShouldThrow()) return Nothing<bool>();
    return _maybe<bool>(JSValueToBoolean(ctx, r)).toMaybe();
}

Maybe<bool> v8::Map::Delete(Local<v8::Context> context,
                            Local<v8::Value> key)
{
    IsolateImpl* iso = ToIsolateImpl(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    LocalException exception(iso);
    JSValueRef args[] = {
        obj,
        ToJSValueRef(key, context)
    };
    JSValueRef r = exec(ctx, "return _1.delete(_2)", 2, args, &exception);
    if (exception.ShouldThrow()) return Nothing<bool>();
    return _maybe<bool>(JSValueToBoolean(ctx, r)).toMaybe();
}

/**
 * Returns an array of length Size() * 2, where index N is the Nth key and
 * index N + 1 is the Nth value.
 */
Local<v8::Array> v8::Map::AsArray() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    return V82JSC::Value::New(ToContextImpl(context), exec(ctx,
            "var r = []; _1.forEach((v,k,m)=>r.push(k,v)); return r",
            1, &obj)).As<Array>();
}

/**
 * Creates a new empty Map.
 */
Local<v8::Map> v8::Map::New(Isolate* isolate)
{
    Local<Context> context = OperatingContext(isolate);
    
    return V82JSC::Value::New(ToContextImpl(context),
                          exec(ToContextRef(context), "return new Map()", 0, 0)).As<Map>();
}
