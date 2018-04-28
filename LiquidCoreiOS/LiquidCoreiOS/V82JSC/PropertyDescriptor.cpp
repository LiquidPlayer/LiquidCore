//
//  PropertyDescriptor.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/5/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

// GenericDescriptor
PropertyDescriptor::PropertyDescriptor()
{
    assert(0);
}

// DataDescriptor
PropertyDescriptor::PropertyDescriptor(Local<Value> value)
{
    assert(0);
}

// DataDescriptor with writable property
PropertyDescriptor::PropertyDescriptor(Local<Value> value, bool writable)
{
    assert(0);
}

// AccessorDescriptor
PropertyDescriptor::PropertyDescriptor(Local<Value> get, Local<Value> set)
{
    assert(0);
}

PropertyDescriptor::~PropertyDescriptor()
{
    assert(0);
}

Local<Value> PropertyDescriptor::value() const
{
    assert(0);
    return Local<Value>();
}
bool PropertyDescriptor::has_value() const
{
    assert(0);
    return false;
}

Local<Value> PropertyDescriptor::get() const
{
    assert(0);
    return Local<Value>();
}
bool PropertyDescriptor::has_get() const
{
    assert(0);
    return false;
}
Local<Value> PropertyDescriptor::set() const
{
    assert(0);
    return Local<Value>();
}
bool PropertyDescriptor::has_set() const
{
    assert(0);
    return false;
}

void PropertyDescriptor::set_enumerable(bool enumerable)
{
    assert(0);
}
bool PropertyDescriptor::enumerable() const
{
    assert(0);
    return false;
}
bool PropertyDescriptor::has_enumerable() const
{
    assert(0);
    return false;
}

void PropertyDescriptor::set_configurable(bool configurable)
{
    assert(0);
}
bool PropertyDescriptor::configurable() const
{
    assert(0);
    return false;
}
bool PropertyDescriptor::has_configurable() const
{
    assert(0);
    return false;
}

bool PropertyDescriptor::writable() const
{
    assert(0);
    return false;
}
bool PropertyDescriptor::has_writable() const
{
    assert(0);
    return false;
}

