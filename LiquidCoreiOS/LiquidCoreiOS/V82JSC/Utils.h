//
//  Utils.hpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Utils_hpp
#define Utils_hpp

#include <Value.h>
#include <Script.h>

class v8::Utils {
public:
    static Local<v8::Value> NewValue(ValueImpl *);
    static Local<v8::Script> NewScript(ScriptImpl *);
};

#endif /* Utils_hpp */
