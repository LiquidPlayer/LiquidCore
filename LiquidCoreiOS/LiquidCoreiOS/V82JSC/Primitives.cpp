//
//  Number.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

double Number::Value() const
{
    return 0.0;
}

Local<Number> Number::New(Isolate* isolate, double value)
{
    return Local<Number>();
}

Local<Integer> Integer::New(Isolate* isolate, int32_t value)
{
    return Local<Integer>();
}
Local<Integer> Integer::NewFromUnsigned(Isolate* isolate, uint32_t value)
{
    return Local<Integer>();
}
int64_t Integer::Value() const
{
    return 0;
}

int32_t Int32::Value() const
{
    return 0;
}

uint32_t Uint32::Value() const
{
    return 0;
}

bool v8::Boolean::Value() const
{
    return false;
}

/**
 * Returns the identity hash for this object. The current implementation
 * uses an inline property on the object to store the identity hash.
 *
 * The return value will never be 0. Also, it is not guaranteed to be
 * unique.
 */
int Name::GetIdentityHash()
{
    return 1;
}

v8::Primitive * UndefinedImpl::New(v8::Isolate *isolate)
{
    UndefinedImpl *undefined = (UndefinedImpl*) malloc(sizeof(UndefinedImpl));
    memset(undefined, 0, sizeof(UndefinedImpl));
    undefined->pMap = &undefined->map;
    undefined->map.set_instance_type(v8::internal::ODDBALL_TYPE);
    internal::Oddball* oddball_handle = reinterpret_cast<internal::Oddball*>(reinterpret_cast<intptr_t>(undefined) + 1);
    oddball_handle->set_kind(internal::Internals::kUndefinedOddballKind);
    
    // init m_undefined?
    return reinterpret_cast<v8::Primitive*>(undefined);
}

