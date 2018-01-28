//
//  Value.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Value.h"

using namespace v8;

Local<Value> ValueImpl::New(ContextImpl *ctx, JSValueRef value)
{
    ValueImpl * impl = (ValueImpl *) malloc(sizeof(ValueImpl));
    memset(impl, 0, sizeof(ValueImpl));
    
    impl->m_context = ctx;
    impl->m_value = value;
    
    //return Local<Value>(impl);
    return Local<Value>();
}

