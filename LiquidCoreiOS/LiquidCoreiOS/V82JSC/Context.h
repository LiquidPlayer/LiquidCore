//
//  Context.h
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Context_h
#define Context_h

#include "Isolate.h"

struct ContextImpl : v8::Context
{
    v8::internal::Context *pInternal;

    JSContextRef m_context;
    IsolateImpl *isolate;
};

#endif /* Context_h */

