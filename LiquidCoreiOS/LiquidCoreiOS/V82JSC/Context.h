//
//  Context.h
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Context_h
#define Context_h

#include <v8.h>
#include <JavaScriptCore/JavaScript.h>

struct ContextImpl : v8::Context
{
    JSContextRef m_context;
};

#endif /* Context_h */

