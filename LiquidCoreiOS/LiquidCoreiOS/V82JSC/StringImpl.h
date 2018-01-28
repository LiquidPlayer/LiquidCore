//
//  StringImpl.hpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef StringImpl_hpp
#define StringImpl_hpp

#include <v8.h>
#include <JavaScriptCore/JavaScript.h>

struct StringImpl : public v8::String {
    JSStringRef m_string;
};

#endif /* StringImpl_hpp */
