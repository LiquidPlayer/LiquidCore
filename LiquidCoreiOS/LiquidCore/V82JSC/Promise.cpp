/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using v8::Local;
using v8::EscapableHandleScope;
using v8::HandleScope;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Isolate;
using v8::Promise;

MaybeLocal<Promise::Resolver> Promise::Resolver::New(Local<Context> context)
{
    Isolate* isolate = ToIsolate(ToContextImpl(context));
    IsolateImpl* iso = ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);
    
    JSContextRef ctx = ToContextRef(context);
    LocalException exception(iso);
    
    JSValueRef resolver = exec(ctx,
                                       "var pr = { }; pr.promise = new Promise((resolve,reject) => { pr.resolve = resolve; pr.reject = reject; }); return pr;",
                                       0, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<Promise::Resolver>();
    }

    JSValueRef promise = exec(ctx, "return _1.promise", 1, &resolver);

    Local<Promise::Resolver> local = V82JSC::Value::New(ToContextImpl(context),
                                                    promise, iso->m_promise_resolver_map).As<Promise::Resolver>();
    auto impl = ToImpl<V82JSC::Value>(local);
    impl->m_secondary_value = resolver;
    JSValueProtect(ctx, impl->m_secondary_value);
    
    return scope.Escape(local);
}
    
/**
 * Extract the associated promise.
 */
Local<Promise> Promise::Resolver::GetPromise()
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    auto impl = ToImpl<V82JSC::Value>(this);

    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef resolver = impl->m_secondary_value;
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context),
                                           exec(ctx, "return _1.promise", 1, &resolver)).As<Promise>());
}

/**
 * Resolve/reject the associated promise with a given value.
 * Ignored if the promise is no longer pending.
 */
Maybe<bool> Promise::Resolver::Resolve(Local<Context> context,Local<Value> value)
{
    Isolate* isolate = ToIsolate(ToContextImpl(context));
    IsolateImpl* iso = ToIsolateImpl(isolate);
    HandleScope scope(isolate);
    auto impl = ToImpl<V82JSC::Value>(this);

    LocalException exception(iso);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        impl->m_secondary_value,
        ToJSValueRef(value, context)
    };
    JSValueRef success = exec(ctx, "return _1.resolve(_2)", 2, args, &exception);
    if (!exception.ShouldThrow()) {
        return _maybe<bool>(JSValueToBoolean(ctx, success)).toMaybe();
    }
    return Nothing<bool>();
}

Maybe<bool> Promise::Resolver::Reject(Local<Context> context, Local<Value> value)
{
    Isolate* isolate = ToIsolate(ToContextImpl(context));
    IsolateImpl* iso = ToIsolateImpl(isolate);
    HandleScope scope(isolate);
    auto impl = ToImpl<V82JSC::Value>(this);

    LocalException exception(iso);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        impl->m_secondary_value,
        ToJSValueRef(value, context)
    };
    JSValueRef success = exec(ctx, "return _1.reject(_2)", 2, args, &exception);
    if (!exception.ShouldThrow()) {
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
    Isolate* isolate = ToIsolate(ToContextImpl(context));
    IsolateImpl* iso = ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);

    LocalException exception(iso);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(this, context),
        ToJSValueRef(handler, context)
    };
    JSValueRef promise = exec(ctx, "return _1.catch(_2)", 2, args, &exception);
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), promise).As<Promise>());
    }
    return MaybeLocal<Promise>();
}

MaybeLocal<Promise> Promise::Then(Local<Context> context, Local<Function> handler)
{
    Isolate* isolate = ToIsolate(ToContextImpl(context));
    IsolateImpl* iso = ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);

    LocalException exception(iso);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ToJSValueRef(this, context),
        ToJSValueRef(handler, context)
    };
    JSValueRef promise = exec(ctx, "return _1.then(_2)", 2, args, &exception);
    if (!exception.ShouldThrow()) {
        return scope.Escape(V82JSC::Value::New(ToContextImpl(context), promise).As<Promise>());
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
Local<v8::Value> Promise::Result()
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    //enum PromiseState { kPending, kFulfilled, kRejected };
    Local<Context> context = ToCurrentContext(this);
    
    Local<Value> result = Get(context,
                             String::NewFromUtf8(isolate, "_value", NewStringType::kNormal).ToLocalChecked())
    .ToLocalChecked();
    return scope.Escape(result);
}

/**
 * Returns the value of the [[PromiseState]] field.
 */
Promise::PromiseState Promise::State()
{
    Isolate* isolate = ToIsolate(this);
    HandleScope scope(isolate);

    //enum PromiseState { kPending, kFulfilled, kRejected };
    Local<Context> context = ToCurrentContext(this);
    
    Local<Value> state = Get(context,
                             String::NewFromUtf8(isolate, "_state", NewStringType::kNormal).ToLocalChecked())
    .ToLocalChecked();
    
    int _state = state->Int32Value(context).ToChecked();
    if (_state == 3) _state = 0;
    return static_cast<PromiseState>(_state);
}
