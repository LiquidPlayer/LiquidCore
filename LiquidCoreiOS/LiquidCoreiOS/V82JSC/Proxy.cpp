//
//  Proxy.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Local<Object> Proxy::GetTarget()
{
    return Local<Object>();
}
Local<Value> Proxy::GetHandler()
{
    return Local<Value>();
}
bool Proxy::IsRevoked()
{
    return false;
}
void Proxy::Revoke()
{

}

/**
 * Creates a new Proxy for the target object.
 */
MaybeLocal<Proxy> Proxy::New(Local<Context> context,
                             Local<Object> local_target,
                             Local<Object> local_handler)
{
    return MaybeLocal<Proxy>();
}
