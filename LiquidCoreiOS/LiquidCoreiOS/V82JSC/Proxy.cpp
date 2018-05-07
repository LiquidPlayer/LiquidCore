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
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSValueRef revoked = V82JSC::exec(ctx,
                                      "try { new Proxy(_1, _1); return false; } catch (err) { return Object(_1) === value; }",
                                      1, &obj);
    return JSValueToBoolean(ctx, revoked);
}
void Proxy::Revoke()
{
    assert(0); // FIXME: We need to handle proxies differently (using Proxy.revocable())
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    V82JSC::exec(ctx, "_1.revoke()", 1, &obj);
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
    LocalException exception(V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context)));
    JSValueRef proxy = V82JSC::exec(V82JSC::ToContextRef(context), "return new Proxy(_1, _2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), proxy).As<Proxy>();
    }
    return MaybeLocal<Proxy>();
}
