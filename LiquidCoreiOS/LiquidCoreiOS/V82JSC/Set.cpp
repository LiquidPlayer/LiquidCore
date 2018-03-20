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
    return 0;
}
void Set::Clear()
{
    
}
MaybeLocal<Set> Set::Add(Local<Context> context, Local<Value> key)
{
    return MaybeLocal<Set>();
}
Maybe<bool> Set::Has(Local<Context> context, Local<Value> key)
{
    return Nothing<bool>();
}
Maybe<bool> Set::Delete(Local<Context> context, Local<Value> key)
{
    return Nothing<bool>();
}

/**
 * Returns an array of the keys in this Set.
 */
Local<Array> Set::AsArray() const
{
    return Local<Array>();
}

/**
 * Creates a new empty Set.
 */
Local<Set> Set::New(Isolate* isolate)
{
    return Local<Set>();
}
