/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using namespace v8;

size_t Set::Size() const
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
void Set::Clear()
{
    reinterpret_cast<Map*>(this)->Clear();
}
MaybeLocal<Set> Set::Add(Local<Context> context, Local<Value> key)
{
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    IsolateImpl* iso = ToIsolateImpl(this);

    LocalException exception(iso);
    auto impl = ToImpl<V82JSC::Value, Set>(this);
    JSValueRef args[] = {
        obj,
        ToJSValueRef(key, context),
    };
    exec(ctx, "_1.add(_2)", 2, args, &exception);
    if (exception.ShouldThrow()) return MaybeLocal<Set>();
    return CreateLocal<Set>(ToIsolate(iso), impl);
}
Maybe<bool> Set::Has(Local<Context> context, Local<Value> key)
{
    return reinterpret_cast<Map*>(this)->Has(context, key);
}
Maybe<bool> Set::Delete(Local<Context> context, Local<Value> key)
{
    return reinterpret_cast<Map*>(this)->Delete(context, key);
}

/**
 * Returns an array of the keys in this Set.
 */
Local<Array> Set::AsArray() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    return V82JSC::Value::New(ToContextImpl(context),
                          exec(ctx, "var r = []; _1.forEach((v)=>r.push(v)); return r", 1, &obj)).As<Array>();
}

/**
 * Creates a new empty Set.
 */
Local<Set> Set::New(Isolate* isolate)
{
    Local<Context> context = OperatingContext(isolate);
    return V82JSC::Value::New(ToContextImpl(context),
                          exec(ToContextRef(context), "return new Set()", 0, 0)).As<Set>();
}
