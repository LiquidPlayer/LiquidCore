//
//  Proxy.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
