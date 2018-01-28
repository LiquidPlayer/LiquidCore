//
//  Value.hpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Value_h
#define Value_h

#include "Context.h"

struct ValueImpl : v8::Value {
    JSValueRef m_value;
    ContextImpl *m_context;
    
    static v8::Local<Value> New(ContextImpl *ctx, JSValueRef value);
};

#endif /* Value_hpp */

