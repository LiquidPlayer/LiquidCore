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

size_t v8::Map::Size() const
{
    HandleScope scope(ToIsolate(this));
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
    HandleScope scope(ToIsolate(this));
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    exec(ctx, "_1.clear()", 1, &obj);
}

MaybeLocal<v8::Value> v8::Map::Get(Local<Context> context, Local<Value> key)
{
    EscapableHandleScope scope(ToIsolate(this));
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
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), r));
}

MaybeLocal<v8::Map> v8::Map::Set(Local<v8::Context> context,
                                 Local<v8::Value> key,
                                 Local<v8::Value> value)
{
    EscapableHandleScope scope(ToIsolate(this));
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
    return scope.Escape(CreateLocal<v8::Map>(ToIsolate(iso), impl));
}

Maybe<bool> v8::Map::Has(Local<v8::Context> context, Local<v8::Value> key)
{
    HandleScope scope(ToIsolate(this));
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
    HandleScope scope(ToIsolate(this));
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
    EscapableHandleScope scope(ToIsolate(this));
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), exec(ctx,
            "var r = []; _1.forEach((v,k,m)=>r.push(k,v)); return r",
            1, &obj)).As<Array>());
}

/**
 * Creates a new empty Map.
 */
Local<v8::Map> v8::Map::New(Isolate* isolate)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = OperatingContext(isolate);
    
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                          exec(ToContextRef(context), "return new Map()", 0, 0)).As<Map>());
}
