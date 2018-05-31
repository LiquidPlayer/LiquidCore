//
//  Exec.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/31/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

JSObjectRef V82JSC::make_exec_function(JSGlobalContextRef gctx, const char *body, int argc)
{
    IsolateImpl* iso = IsolateImpl::s_context_to_isolate_map[gctx];
    JSValueRef exception = 0;

    JSStringRef argNames[argc];
    JSStringRef anon = JSStringCreateWithUTF8CString("anon");
    JSStringRef sbody = JSStringCreateWithUTF8CString(body);
    for (int i=0; i<argc; i++) {
        char argname[64];
        sprintf(argname, "_%d", i+1);
        argNames[i] = JSStringCreateWithUTF8CString(argname);
    }
    JSObjectRef function = JSObjectMakeFunction(gctx, anon, argc, argNames, sbody, 0, 0, &exception);
    for (int i=0; i<argc; i++) {
        JSStringRelease(argNames[i]);
    }
    assert(exception==0);
    iso->m_exec_maps[gctx][body] = function;
    JSValueProtect(gctx, function);
    JSStringRelease(anon);
    JSStringRelease(sbody);

    return function;
}
