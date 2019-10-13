/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

struct PropertyDescriptor::PrivateData
{
    Local<Value> value;
    Local<Value> get;
    Local<Value> set;
    bool enumerable;
    bool has_enumerable;
    bool writable;
    bool has_writable;
    bool configurable;
    bool has_configurable;
};

// GenericDescriptor
PropertyDescriptor::PropertyDescriptor()
{
    private_ = new PrivateData();
}

// DataDescriptor
PropertyDescriptor::PropertyDescriptor(Local<Value> value)
{
    private_ = new PrivateData();
    private_->value = value;
}

// DataDescriptor with writable property
PropertyDescriptor::PropertyDescriptor(Local<Value> value, bool writable)
{
    private_ = new PrivateData();
    private_->value = value;
    private_->has_writable = true;
    private_->writable = writable;
}

// AccessorDescriptor
PropertyDescriptor::PropertyDescriptor(Local<Value> get, Local<Value> set)
{
    private_ = new PrivateData();
    private_->get = get;
    private_->set = set;
}

PropertyDescriptor::~PropertyDescriptor()
{
    delete private_;
}

Local<v8::Value> PropertyDescriptor::value() const
{
    return private_->value;
}
bool PropertyDescriptor::has_value() const
{
    return !private_->value.IsEmpty();
}

Local<v8::Value> PropertyDescriptor::get() const
{
    return private_->get;
}
bool PropertyDescriptor::has_get() const
{
    return !private_->get.IsEmpty();
}
Local<v8::Value> PropertyDescriptor::set() const
{
    return private_->set;
}
bool PropertyDescriptor::has_set() const
{
    return !private_->set.IsEmpty();
}

void PropertyDescriptor::set_enumerable(bool enumerable)
{
    private_->has_enumerable = true;
    private_->enumerable = enumerable;
}
bool PropertyDescriptor::enumerable() const
{
    return private_->enumerable;
}
bool PropertyDescriptor::has_enumerable() const
{
    return private_->has_enumerable;
}

void PropertyDescriptor::set_configurable(bool configurable)
{
    private_->has_configurable = true;
    private_->configurable = configurable;
}
bool PropertyDescriptor::configurable() const
{
    return private_->configurable;
}
bool PropertyDescriptor::has_configurable() const
{
    return private_->has_configurable;
}

bool PropertyDescriptor::writable() const
{
    return private_->writable;
}
bool PropertyDescriptor::has_writable() const
{
    return private_->has_writable;
}

