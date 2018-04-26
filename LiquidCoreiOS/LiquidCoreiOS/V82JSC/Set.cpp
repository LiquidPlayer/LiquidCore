//
//  Set.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/8/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

size_t Set::Size() const
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Set>(this);
    JSStringRef ssize = JSStringCreateWithUTF8CString("size");
    JSValueRef excp = 0;
    JSValueRef size = JSObjectGetProperty(impl->m_context->m_ctxRef, (JSObjectRef)impl->m_value, ssize, &excp);
    assert(excp==0);
    return JSValueToNumber(impl->m_context->m_ctxRef, size, 0);
}
void Set::Clear()
{
    _local<Map>(this).toLocal()->Clear();
}
MaybeLocal<Set> Set::Add(Local<Context> context, Local<Value> key)
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Set>(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(impl->m_isolate);
    JSValueRef args[] = {
        impl->m_value,
        V82JSC::ToJSValueRef(key, context),
    };
    V82JSC::exec(ctx, "_1.add(_2)", 2, args, &exception);
    if (exception.ShouldThow()) return MaybeLocal<Set>();
    return _local<Set>(this).toLocal();
}
Maybe<bool> Set::Has(Local<Context> context, Local<Value> key)
{
    return _local<Map>(this).toLocal()->Has(context, key);
}
Maybe<bool> Set::Delete(Local<Context> context, Local<Value> key)
{
    return _local<Map>(this).toLocal()->Delete(context, key);
}

/**
 * Returns an array of the keys in this Set.
 */
Local<Array> Set::AsArray() const
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Set>(this);
    return ValueImpl::New(impl->m_context,
                          V82JSC::exec(impl->m_context->m_ctxRef,
                                       "var r = []; _1.forEach((v)=>r.push(v)); return r",
                                       1, &impl->m_value)).As<Array>();
}

/**
 * Creates a new empty Set.
 */
Local<Set> Set::New(Isolate* isolate)
{
    return ValueImpl::New(V82JSC::ToIsolateImpl(isolate)->m_defaultContext,
                          V82JSC::exec(V82JSC::ToContextRef(isolate),
                                       "return new Set()", 0, 0)).As<Set>();
}
