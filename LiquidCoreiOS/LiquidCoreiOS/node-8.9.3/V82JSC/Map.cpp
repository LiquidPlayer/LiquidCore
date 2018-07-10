//
//  Map.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/8/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<NativeWeakMap> NativeWeakMap::New(Isolate* isolate)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    
    Local<Value> nwm = ValueImpl::New(V82JSC::ToContextImpl(context),
        V82JSC::exec(V82JSC::ToContextRef(context),
        "return new WeakMap()", 0, 0));
    
    Local<NativeWeakMap> loc = * (reinterpret_cast<Local<NativeWeakMap> *>(&nwm));
    return loc;
}
void NativeWeakMap::Set(Local<Value> key, Local<Value> value)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(key, context),
        V82JSC::ToJSValueRef(value, context)
    };
    V82JSC::exec(ctx, "_1[_2] = _3", 3, args);
}
Local<Value> NativeWeakMap::Get(Local<Value> key) const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(key, context),
    };
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return _1[_2]", 2, args));
}
bool NativeWeakMap::Has(Local<Value> key)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(key, context),
    };
    return JSValueToBoolean(ctx, V82JSC::exec(ctx, "return Object.getOwnPropertyDescriptor(_1,_2) !== undefined", 2, args));
}
bool NativeWeakMap::Delete(Local<Value> key)
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(key, context),
    };
    return JSValueToBoolean(ctx, V82JSC::exec(ctx, "return delete _1[_2]", 2, args));
}


size_t Map::Size() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);

    JSStringRef ssize = JSStringCreateWithUTF8CString("size");
    JSValueRef excp = 0;
    JSValueRef size = JSObjectGetProperty(ctx, (JSObjectRef)obj, ssize, &excp);
    assert(excp==0);
    return JSValueToNumber(ctx, size, 0);
}

void Map::Clear()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    V82JSC::exec(ctx, "_1.clear()", 1, &obj);
}

MaybeLocal<Value> Map::Get(Local<Context> context, Local<Value> key)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    LocalException exception(iso);
    JSValueRef args[] = {
        obj,
        V82JSC::ToJSValueRef(key, context)
    };
    JSValueRef r = V82JSC::exec(ctx, "return _1.get(_2)", 2, args, &exception);
    if (exception.ShouldThow()) return MaybeLocal<Value>();
    return ValueImpl::New(V82JSC::ToContextImpl(context), r);
}

MaybeLocal<Map> Map::Set(Local<Context> context,
                         Local<Value> key,
                         Local<Value> value)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    LocalException exception(iso);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl, Map>(this);
    JSValueRef args[] = {
        obj,
        V82JSC::ToJSValueRef(key, context),
        V82JSC::ToJSValueRef(value, context)
    };
    V82JSC::exec(ctx, "_1.set(_2, _3)", 3, args, &exception);
    if (exception.ShouldThow()) return MaybeLocal<Map>();
    return V82JSC::CreateLocal<Map>(V82JSC::ToIsolate(iso), impl);
}

Maybe<bool> Map::Has(Local<Context> context, Local<Value> key)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    LocalException exception(iso);
    JSValueRef args[] = {
        obj,
        V82JSC::ToJSValueRef(key, context)
    };
    JSValueRef r = V82JSC::exec(ctx, "return _1.has(_2)", 2, args, &exception);
    if (exception.ShouldThow()) return Nothing<bool>();
    return _maybe<bool>(JSValueToBoolean(ctx, r)).toMaybe();
}

Maybe<bool> Map::Delete(Local<Context> context,
                                         Local<Value> key)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    LocalException exception(iso);
    JSValueRef args[] = {
        obj,
        V82JSC::ToJSValueRef(key, context)
    };
    JSValueRef r = V82JSC::exec(ctx, "return _1.delete(_2)", 2, args, &exception);
    if (exception.ShouldThow()) return Nothing<bool>();
    return _maybe<bool>(JSValueToBoolean(ctx, r)).toMaybe();
}

/**
 * Returns an array of length Size() * 2, where index N is the Nth key and
 * index N + 1 is the Nth value.
 */
Local<Array> Map::AsArray() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    return ValueImpl::New(V82JSC::ToContextImpl(context),
                          V82JSC::exec(ctx,
                                       "var r = []; _1.forEach((v,k,m)=>r.push(k,v)); return r",
                                       1, &obj)).As<Array>();
}

/**
 * Creates a new empty Map.
 */
Local<Map> Map::New(Isolate* isolate)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    
    return ValueImpl::New(V82JSC::ToContextImpl(context),
                          V82JSC::exec(V82JSC::ToContextRef(context),
                                       "return new Map()", 0, 0)).As<Map>();
}
