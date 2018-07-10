//
//  Promise.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

MaybeLocal<Promise::Resolver> Promise::Resolver::New(Local<Context> context)
{
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);
    
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(iso);
    
    JSValueRef resolver = V82JSC::exec(ctx,
                                       "var pr = { }; pr.promise = new Promise((resolve,reject) => { pr.resolve = resolve; pr.reject = reject; }); return pr;",
                                       0, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<Promise::Resolver>();
    }

    JSValueRef promise = V82JSC::exec(ctx, "return _1.promise", 1, &resolver);

    Local<Promise::Resolver> local = ValueImpl::New(V82JSC::ToContextImpl(context),
                                                    promise, iso->m_promise_resolver_map).As<Promise::Resolver>();
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(local);
    impl->m_secondary_value = resolver;
    JSValueProtect(ctx, impl->m_secondary_value);
    
    return scope.Escape(local);
}
    
/**
 * Extract the associated promise.
 */
Local<Promise> Promise::Resolver::GetPromise()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    EscapableHandleScope scope(isolate);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(this);

    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef resolver = impl->m_secondary_value;
    return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return _1.promise", 1, &resolver)).As<Promise>());
}

/**
 * Resolve/reject the associated promise with a given value.
 * Ignored if the promise is no longer pending.
 */
Maybe<bool> Promise::Resolver::Resolve(Local<Context> context,Local<Value> value)
{
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    HandleScope scope(isolate);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(this);

    LocalException exception(iso);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        impl->m_secondary_value,
        V82JSC::ToJSValueRef(value, context)
    };
    JSValueRef success = V82JSC::exec(ctx, "return _1.resolve(_2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return _maybe<bool>(JSValueToBoolean(ctx, success)).toMaybe();
    }
    return Nothing<bool>();
}

Maybe<bool> Promise::Resolver::Reject(Local<Context> context, Local<Value> value)
{
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    HandleScope scope(isolate);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(this);

    LocalException exception(iso);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        impl->m_secondary_value,
        V82JSC::ToJSValueRef(value, context)
    };
    JSValueRef success = V82JSC::exec(ctx, "return _1.reject(_2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return _maybe<bool>(JSValueToBoolean(ctx, success)).toMaybe();
    }
    return Nothing<bool>();
}

/**
 * Register a resolution/rejection handler with a promise.
 * The handler is given the respective resolution/rejection value as
 * an argument. If the promise is already resolved/rejected, the handler is
 * invoked at the end of turn.
 */
MaybeLocal<Promise> Promise::Catch(Local<Context> context,Local<Function> handler)
{
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);

    LocalException exception(iso);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(handler, context)
    };
    JSValueRef promise = V82JSC::exec(ctx, "return _1.catch(_2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(context), promise).As<Promise>());
    }
    return MaybeLocal<Promise>();
}

MaybeLocal<Promise> Promise::Then(Local<Context> context, Local<Function> handler)
{
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);

    LocalException exception(iso);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(handler, context)
    };
    JSValueRef promise = V82JSC::exec(ctx, "return _1.then(_2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(context), promise).As<Promise>());
    }
    return MaybeLocal<Promise>();
}

/**
 * Returns true if the promise has at least one derived promise, and
 * therefore resolve/reject handlers (including default handler).
 */
bool Promise::HasHandler()
{
    assert(0);
    return false;
}

/**
 * Returns the content of the [[PromiseResult]] field. The Promise must not
 * be pending.
 */
Local<Value> Promise::Result()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    //enum PromiseState { kPending, kFulfilled, kRejected };
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        JSObjectMake(ctx, 0, 0)
    };
    V82JSC::exec(ctx,
                 "_1.then(v => { _2.r = v; }, v => { _2.r = v; });",
                 2, args);
    JSValueRef value = V82JSC::exec(ctx,
                                    "return _2.r;", 2, args);
    
    return scope.Escape(ValueImpl::New(V82JSC::ToContextImpl(context), value));
}

/**
 * Returns the value of the [[PromiseState]] field.
 */
Promise::PromiseState Promise::State()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    HandleScope scope(isolate);

    //enum PromiseState { kPending, kFulfilled, kRejected };
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        JSObjectMake(ctx, 0, 0)
    };
    V82JSC::exec(ctx,
                 "const t = {};"
                 "Promise.race([_1, t])"
                 "    .then(v => (v === t)? 0 : 1, () => 2)"
                 "    .then(v => { _2.r = v; });",
                 2, args);
    JSValueRef state = V82JSC::exec(ctx,
                                    "return _2.r;", 2, args);

    return static_cast<PromiseState>(JSValueToNumber(ctx, state, nullptr));
}
