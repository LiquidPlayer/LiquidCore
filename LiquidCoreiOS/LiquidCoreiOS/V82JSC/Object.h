//
//  Object.hpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/4/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Object_h
#define Object_h

#include <v8.h>
#include <JavaScriptCore/JavaScript.h>
#include "Value.h"

struct ObjectImpl : public ValueImpl, v8::Object {
    JSObjectRef m_object;
    
    static v8::Local<v8::Object> New(ContextImpl *ctx, JSObjectRef object);
};

#endif /* Object_h */
