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
    JSContextRef ctx = V82JSC::ToContextRef(context);
    LocalException exception(V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context)));
    
    JSValueRef resolver = V82JSC::exec(ctx,
                                       "var pr = { }; pr.promise = new Promise((resolve,reject) => { pr.resolve = resolve; pr.reject = reject; }); return pr;",
                                       0, nullptr/*, &exception*/);
    if (exception.ShouldThow()) {
        return MaybeLocal<Promise::Resolver>();
    }
    return ValueImpl::New(V82JSC::ToContextImpl(context), resolver).As<Promise::Resolver>();
}
    
/**
 * Extract the associated promise.
 */
Local<Promise> Promise::Resolver::GetPromise()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef resolver = V82JSC::ToJSValueRef(this, context);
    return ValueImpl::New(V82JSC::ToContextImpl(context), V82JSC::exec(ctx, "return _1.promise", 1, &resolver)).As<Promise>();
}

/**
 * Resolve/reject the associated promise with a given value.
 * Ignored if the promise is no longer pending.
 */
Maybe<bool> Promise::Resolver::Resolve(Local<Context> context,Local<Value> value)
{
    LocalException exception(V82JSC::ToIsolateImpl(this));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
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
    LocalException exception(V82JSC::ToIsolateImpl(this));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
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
    LocalException exception(V82JSC::ToIsolateImpl(this));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(handler, context)
    };
    JSValueRef promise = V82JSC::exec(ctx, "return _1.catch(_2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), promise).As<Promise>();
    }
    return MaybeLocal<Promise>();
}

MaybeLocal<Promise> Promise::Then(Local<Context> context, Local<Function> handler)
{
    LocalException exception(V82JSC::ToIsolateImpl(this));
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(this, context),
        V82JSC::ToJSValueRef(handler, context)
    };
    JSValueRef promise = V82JSC::exec(ctx, "return _1.then(_2)", 2, args, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), promise).As<Promise>();
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
    assert(0);
    return Local<Value>();
}

/**
 * Returns the value of the [[PromiseState]] field.
 */
Promise::PromiseState Promise::State()
{
    //enum PromiseState { kPending, kFulfilled, kRejected };
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef promise = V82JSC::ToJSValueRef(this, context);
    JSValueRef state = V82JSC::exec(ctx,
                                    "const t = {};"
                                    "return Promise.race([_1, t])"
                                    "    .then(v => (v === t)? 0 : 1, () => 2);",
                                    1, &promise);
    return static_cast<PromiseState>(JSValueToNumber(ctx, state, nullptr));
}
