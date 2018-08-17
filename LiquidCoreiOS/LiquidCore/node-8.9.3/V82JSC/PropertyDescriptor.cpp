//
//  PropertyDescriptor.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

Local<Value> PropertyDescriptor::value() const
{
    return private_->value;
}
bool PropertyDescriptor::has_value() const
{
    return !private_->value.IsEmpty();
}

Local<Value> PropertyDescriptor::get() const
{
    return private_->get;
}
bool PropertyDescriptor::has_get() const
{
    return !private_->get.IsEmpty();
}
Local<Value> PropertyDescriptor::set() const
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

