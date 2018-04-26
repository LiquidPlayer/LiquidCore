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
    assert(0);
    return Local<NativeWeakMap>();
}
void NativeWeakMap::Set(Local<Value> key, Local<Value> value)
{
    assert(0);
}
Local<Value> NativeWeakMap::Get(Local<Value> key) const
{
    assert(0);
    return Local<Value>();
}
bool NativeWeakMap::Has(Local<Value> key)
{
    assert(0);
    return false;
}
bool NativeWeakMap::Delete(Local<Value> key)
{
    assert(0);
    return false;
}


size_t Map::Size() const
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    JSStringRef ssize = JSStringCreateWithUTF8CString("size");
    JSValueRef excp = 0;
    JSValueRef size = JSObjectGetProperty(impl->m_context->m_ctxRef, (JSObjectRef)impl->m_value, ssize, &excp);
    assert(excp==0);
    return JSValueToNumber(impl->m_context->m_ctxRef, size, 0);
}

void Map::Clear()
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    V82JSC::exec(impl->m_context->m_ctxRef, "_1.clear()", 1, &impl->m_value);
}

MaybeLocal<Value> Map::Get(Local<Context> context, Local<Value> key)
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    LocalException exception(impl->m_isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        impl->m_value,
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
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    LocalException exception(impl->m_isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        impl->m_value,
        V82JSC::ToJSValueRef(key, context),
        V82JSC::ToJSValueRef(value, context)
    };
    V82JSC::exec(ctx, "_1.set(_2, _3)", 3, args, &exception);
    if (exception.ShouldThow()) return MaybeLocal<Map>();
    return _local<Map>(this).toLocal();
}

Maybe<bool> Map::Has(Local<Context> context, Local<Value> key)
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(impl->m_isolate);
    JSValueRef args[] = {
        impl->m_value,
        V82JSC::ToJSValueRef(key, context)
    };
    JSValueRef r = V82JSC::exec(ctx, "return _1.has(_2)", 2, args, &exception);
    if (exception.ShouldThow()) return Nothing<bool>();
    return _maybe<bool>(JSValueToBoolean(ctx, r)).toMaybe();
}

Maybe<bool> Map::Delete(Local<Context> context,
                                         Local<Value> key)
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(impl->m_isolate);
    JSValueRef args[] = {
        impl->m_value,
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
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Map>(this);
    return ValueImpl::New(impl->m_context,
                          V82JSC::exec(impl->m_context->m_ctxRef,
                                       "var r = []; _1.forEach((v,k,m)=>r.push(k,v)); return r",
                                       1, &impl->m_value)).As<Array>();
}

/**
 * Creates a new empty Map.
 */
Local<Map> Map::New(Isolate* isolate)
{
    return ValueImpl::New(V82JSC::ToIsolateImpl(isolate)->m_defaultContext,
                          V82JSC::exec(V82JSC::ToContextRef(isolate),
                                       "return new Map()", 0, 0)).As<Map>();
}
