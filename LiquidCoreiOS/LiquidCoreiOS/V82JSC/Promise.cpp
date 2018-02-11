//
//  Promise.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Promise.h"

using namespace v8;

MaybeLocal<Promise::Resolver> Promise::Resolver::New(Local<Context> context)
{
    return MaybeLocal<Promise::Resolver>();
}
    
/**
 * Extract the associated promise.
 */
Local<Promise> Promise::Resolver::GetPromise()
{
    return Local<Promise>();
}

/**
 * Resolve/reject the associated promise with a given value.
 * Ignored if the promise is no longer pending.
 */
Maybe<bool> Promise::Resolver::Resolve(Local<Context> context,Local<Value> value)
{
    return Nothing<bool>();
}

Maybe<bool> Promise::Resolver::Reject(Local<Context> context, Local<Value> value)
{
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
    return MaybeLocal<Promise>();
}

MaybeLocal<Promise> Promise::Then(Local<Context> context, Local<Function> handler)
{
    return MaybeLocal<Promise>();
}

/**
 * Returns true if the promise has at least one derived promise, and
 * therefore resolve/reject handlers (including default handler).
 */
bool Promise::HasHandler()
{
    return false;
}

/**
 * Returns the content of the [[PromiseResult]] field. The Promise must not
 * be pending.
 */
Local<Value> Promise::Result()
{
    return Local<Value>();
}

/**
 * Returns the value of the [[PromiseState]] field.
 */
Promise::PromiseState Promise::State()
{
    return PromiseState();
}
