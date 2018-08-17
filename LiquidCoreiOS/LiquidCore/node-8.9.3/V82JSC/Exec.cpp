//
//  Exec.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "V82JSC.h"

using namespace v8;

JSObjectRef V82JSC::make_exec_function(JSGlobalContextRef gctx, const char *body, int argc)
{
    IsolateImpl* iso;
    {
        std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
        iso = IsolateImpl::s_context_to_isolate_map[gctx];
    }
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
