//
//  Isolate.h
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Isolate_h
#define Isolate_h

#include <v8.h>
#include <JavaScriptCore/JavaScript.h>

struct IsolateImpl : v8::Isolate {
    JSContextGroupRef m_group;
};

#endif /* Isolate_h */

