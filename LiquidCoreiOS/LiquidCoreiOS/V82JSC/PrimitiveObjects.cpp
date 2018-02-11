//
//  PrimitiveObjects.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "PrimitiveObjects.h"
#include "Utils.h"

using namespace v8;

Local<Value> NumberObject::New(Isolate* isolate, double value)
{
    return Utils::NewValue(nullptr);
}

double NumberObject::ValueOf() const
{
    return 0.0;
}

Local<Value> BooleanObject::New(Isolate* isolate, bool value)
{
    return Utils::NewValue(nullptr);
}
bool BooleanObject::ValueOf() const
{
    return false;
}

Local<Value> StringObject::New(Local<String> value)
{
    return Utils::NewValue(nullptr);
}

Local<String> StringObject::ValueOf() const
{
    return Local<String>();
}

Local<Value> SymbolObject::New(Isolate* isolate, Local<Symbol> value)
{
    return Utils::NewValue(nullptr);
}

Local<Symbol> SymbolObject::ValueOf() const
{
    return Local<Symbol>();
}

