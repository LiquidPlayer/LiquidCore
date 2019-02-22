/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

using namespace V82JSC;
using namespace v8;

Local<Object> Proxy::GetTarget()
{
    Local<Context> context = ToCurrentContext(this);
    JSValueRef obj = ToJSValueRef(this, context);

    JSObjectRef target = JSObjectGetProxyTarget((JSObjectRef)obj);
    return V82JSC::Value::New(ToContextImpl(context), target).As<Object>();
}

Local<v8::Value> Proxy::GetHandler()
{
    Local<Context> context = ToCurrentContext(this);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    JSValueRef obj = ToJSValueRef(this, context);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        iso->m_proxy_revocables,
        obj
    };
    JSValueRef exception = 0;
    JSValueRef handler = exec(ctx, "return _1[_2]['handler']", 2, args, &exception);
    if (exception) return Null(ToIsolate(iso));
    return V82JSC::Value::New(ToContextImpl(context), handler);
}

bool Proxy::IsRevoked()
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef obj = ToJSValueRef(this, context);
    JSValueRef revoked = exec(ctx,
        "try { new Proxy(_1, _1); return false; } catch (err) { return String(err).includes('revoked'); }",
        1, &obj);
    return JSValueToBoolean(ctx, revoked);
}

void Proxy::Revoke()
{
    Local<Context> context = ToCurrentContext(this);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    JSValueRef obj = ToJSValueRef(this, context);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        iso->m_proxy_revocables,
        obj
    };
    exec(ctx, "_1[_2].revoke()", 2, args);
}

/**
 * Creates a new Proxy for the target object.
 */
MaybeLocal<Proxy> Proxy::New(Local<Context> context,
                             Local<Object> local_target,
                             Local<Object> local_handler)
{
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(local_target, context),
        ToJSValueRef(local_handler, context),
        iso->m_proxy_revocables
    };
    LocalException exception(iso);
    JSValueRef proxy = exec(ctx, "var revocable = Proxy.revocable(_1, _2); revocable.handler = _2;"
                                    "_3[revocable.proxy] = revocable; return revocable.proxy", 3, args, &exception);
    if (!exception.ShouldThow()) {
        return V82JSC::Value::New(ToContextImpl(context), proxy).As<Proxy>();
    }
    return MaybeLocal<Proxy>();
}
