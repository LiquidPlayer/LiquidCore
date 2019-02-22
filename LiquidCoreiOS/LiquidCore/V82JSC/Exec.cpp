/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

namespace V82JSC {

JSObjectRef make_exec_function(JSGlobalContextRef gctx, const char *body, int argc)
{
    IsolateImpl* iso = IsolateFromCtx(gctx);
    JSValueRef exception = 0;

    JSStringRef argNames[argc];
    JSStringRef anon = JSStringCreateWithUTF8CString("anon");
    JSStringRef native_function = JSStringCreateWithUTF8CString("[native code]");
    JSStringRef sbody = JSStringCreateWithUTF8CString(body);
    for (int i=0; i<argc; i++) {
        char argname[64];
        sprintf(argname, "_%d", i+1);
        argNames[i] = JSStringCreateWithUTF8CString(argname);
    }
    JSObjectRef function = JSObjectMakeFunction(gctx, anon, argc, argNames, sbody, native_function, 1, &exception);
    for (int i=0; i<argc; i++) {
        JSStringRelease(argNames[i]);
    }
    if (exception!=0) {
        JSStringRef error = JSValueToStringCopy(gctx, exception, 0);
        char msg[JSStringGetMaximumUTF8CStringSize(error)];
        JSStringGetUTF8CString(error, msg, JSStringGetMaximumUTF8CStringSize(error));
        fprintf(stderr, ">>> %s\n", msg);
    }
    assert(exception==0);
    iso->m_exec_maps[gctx][body] = function;
    JSValueProtect(gctx, function);
    JSStringRelease(anon);
    JSStringRelease(sbody);
    JSStringRelease(native_function);

    return function;
}
}
