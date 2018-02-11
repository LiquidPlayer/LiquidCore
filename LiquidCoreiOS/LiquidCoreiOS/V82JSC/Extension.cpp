//
//  Extension.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Extension.h"

using namespace v8;

Extension::Extension(const char* name,
          const char* source,
          int dep_count,
          const char** deps,
          int source_length)
{
    
}

void V8_EXPORT v8::RegisterExtension(v8::Extension* extension)
{
    
}
