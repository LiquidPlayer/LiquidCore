//
//  os_dependent.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/22/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef os_dependent_h
#define os_dependent_h

#include <JavaScriptCore/JavaScript.h>
#include "v8.h"
#include "uv.h"

JSContextGroupRef os_groupFromIsolate(v8::Isolate *isolate, uv_loop_t* event_loop);
v8::Local<v8::Context> os_newContext(v8::Isolate *isolate, JSContextGroupRef groupRef,
                                     JSGlobalContextRef *ctxRef);
void os_Dispose(JSContextGroupRef group, JSGlobalContextRef ctxRef);

#endif /* os_dependent_h */
