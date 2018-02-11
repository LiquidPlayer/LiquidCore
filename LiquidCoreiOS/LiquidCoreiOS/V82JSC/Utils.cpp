//
//  Utils.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Utils.h"

using namespace v8;

Local<Value> v8::Utils::NewValue(ValueImpl *value)
{
    return Local<Value>(value);
}

Local<Script> v8::Utils::NewScript(ScriptImpl *script)
{
    return Local<Script>(script);
}

Local<Object> v8::Utils::NewObject(ObjectImpl *object)
{
    return Local<Object>(object);
}

