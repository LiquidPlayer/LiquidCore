/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

using namespace v8;

Local<Object> Proxy::GetTarget()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);

    JSObjectRef target = JSObjectGetProxyTarget((JSObjectRef)obj);
    return ValueImpl::New(V82JSC::ToContextImpl(context), target).As<Object>();
}

Local<Value> Proxy::GetHandler()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        iso->m_proxy_revocables,
        obj
    };
    JSValueRef exception = 0;
    JSValueRef handler = V82JSC::exec(ctx, "return _1[_2]['handler']", 2, args, &exception);
    if (exception) return Null(V82JSC::ToIsolate(iso));
    return ValueImpl::New(V82JSC::ToContextImpl(context), handler);
}

bool Proxy::IsRevoked()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSValueRef revoked = V82JSC::exec(ctx,
        "try { new Proxy(_1, _1); return false; } catch (err) { return String(err).includes('revoked'); }",
        1, &obj);
    return JSValueToBoolean(ctx, revoked);
}

void Proxy::Revoke()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));
    JSValueRef obj = V82JSC::ToJSValueRef(this, context);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        iso->m_proxy_revocables,
        obj
    };
    V82JSC::exec(ctx, "_1[_2].revoke()", 2, args);
}

/**
 * Creates a new Proxy for the target object.
 */
MaybeLocal<Proxy> Proxy::New(Local<Context> context,
                             Local<Object> local_target,
                             Local<Object> local_handler)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(local_target, context),
        V82JSC::ToJSValueRef(local_handler, context),
        iso->m_proxy_revocables
    };
    LocalException exception(iso);
    JSValueRef proxy = V82JSC::exec(ctx, "var revocable = Proxy.revocable(_1, _2); revocable.handler = _2;"
                                    "_3[revocable.proxy] = revocable; return revocable.proxy", 3, args, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), proxy).As<Proxy>();
    }
    return MaybeLocal<Proxy>();
}
