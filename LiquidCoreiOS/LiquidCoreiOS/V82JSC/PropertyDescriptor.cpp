//
//  PropertyDescriptor.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/5/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "PropertyDescriptor.h"
#include "Utils.h"

using namespace v8;

// GenericDescriptor
PropertyDescriptor::PropertyDescriptor()
{
    
}

// DataDescriptor
PropertyDescriptor::PropertyDescriptor(Local<Value> value)
{
    
}

// DataDescriptor with writable property
PropertyDescriptor::PropertyDescriptor(Local<Value> value, bool writable)
{
    
}

// AccessorDescriptor
PropertyDescriptor::PropertyDescriptor(Local<Value> get, Local<Value> set)
{
    
}

PropertyDescriptor::~PropertyDescriptor()
{
    
}

Local<Value> PropertyDescriptor::value() const
{
    return Utils::NewValue(nullptr);
}
bool PropertyDescriptor::has_value() const
{
    return false;
}

Local<Value> PropertyDescriptor::get() const
{
    return Utils::NewValue(nullptr);

}
bool PropertyDescriptor::has_get() const
{
    return false;
}
Local<Value> PropertyDescriptor::set() const
{
    return Utils::NewValue(nullptr);
}
bool PropertyDescriptor::has_set() const
{
    return false;
}

void PropertyDescriptor::set_enumerable(bool enumerable)
{
    
}
bool PropertyDescriptor::enumerable() const
{
    return false;
}
bool PropertyDescriptor::has_enumerable() const
{
    return false;
}

void PropertyDescriptor::set_configurable(bool configurable)
{
    
}
bool PropertyDescriptor::configurable() const
{
    return false;
}
bool PropertyDescriptor::has_configurable() const
{
    return false;
}

bool PropertyDescriptor::writable() const
{
    return false;
}
bool PropertyDescriptor::has_writable() const
{
    return false;
}

