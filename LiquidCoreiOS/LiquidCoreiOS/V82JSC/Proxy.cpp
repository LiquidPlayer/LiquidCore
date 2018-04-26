//
//  Proxy.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<Object> Proxy::GetTarget()
{
    assert(0);
    return Local<Object>();
}
Local<Value> Proxy::GetHandler()
{
    assert(0);
    return Local<Value>();
}
bool Proxy::IsRevoked()
{
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Proxy>(this);
    JSValueRef revoked = V82JSC::exec(impl->m_context->m_ctxRef,
                                      "try { new Proxy(_1, _1); return false; } catch (err) { return Object(_1) === value; }",
                                      1, &impl->m_value);
    return JSValueToBoolean(impl->m_context->m_ctxRef, revoked);
}
void Proxy::Revoke()
{
    assert(0); // FIXME: We need to handle proxies differently (using Proxy.revocable())
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Proxy>(this);
    V82JSC::exec(impl->m_context->m_ctxRef,
                 "_1.revoke()",
                 1, &impl->m_value);
}

/**
 * Creates a new Proxy for the target object.
 */
MaybeLocal<Proxy> Proxy::New(Local<Context> context,
                             Local<Object> local_target,
                             Local<Object> local_handler)
{
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(local_target, context),
        V82JSC::ToJSValueRef(local_handler, context)
    };
    LocalException exception(V82JSC::ToContextImpl(context)->m_isolate);
    JSValueRef proxy = V82JSC::exec(V82JSC::ToContextRef(context), "return new Proxy(_1, _2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), proxy).As<Proxy>();
    }
    return MaybeLocal<Proxy>();
}
