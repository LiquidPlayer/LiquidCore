//
//  Map.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/8/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<NativeWeakMap> NativeWeakMap::New(Isolate* isolate)
{
    return Local<NativeWeakMap>();
}
void NativeWeakMap::Set(Local<Value> key, Local<Value> value)
{
    
}
Local<Value> NativeWeakMap::Get(Local<Value> key) const
{
    return Local<Value>();
}
bool NativeWeakMap::Has(Local<Value> key)
{
    return false;
}
bool NativeWeakMap::Delete(Local<Value> key)
{
    return false;
}


size_t Map::Size() const
{
    return 0;
}

void Map::Clear()
{
    
}

MaybeLocal<Value> Map::Get(Local<Context> context, Local<Value> key)
{
    return MaybeLocal<Value>();
}

MaybeLocal<Map> Map::Set(Local<Context> context,
                         Local<Value> key,
                         Local<Value> value)
{
    return MaybeLocal<Map>();
}

Maybe<bool> Map::Has(Local<Context> context, Local<Value> key)
{
    return Nothing<bool>();
}
Maybe<bool> Map::Delete(Local<Context> context,
                                         Local<Value> key)
{
    return Nothing<bool>();
}

/**
 * Returns an array of length Size() * 2, where index N is the Nth key and
 * index N + 1 is the Nth value.
 */
Local<Array> Map::AsArray() const
{
    return Local<Array>();
}

/**
 * Creates a new empty Map.
 */
Local<Map> Map::New(Isolate* isolate)
{
    return Local<Map>();
}
