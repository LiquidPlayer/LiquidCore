//
//  Script.h
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Script_h
#define Script_h

#include "Context.h"

struct ScriptImpl : v8::Script
{
    JSStringRef m_sourceURL;
    int m_startingLineNumber;
    JSStringRef m_script;
};

#endif /* Script_h */
