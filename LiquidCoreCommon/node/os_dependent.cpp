//
//  os_dependent.cpp
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

#include "os_dependent.h"
using namespace v8;

#ifdef __APPLE__

#include "V82JSC.h"

JSContextGroupRef os_groupFromIsolate(v8::Isolate *isolate, uv_loop_t* event_loop)
{
    return V82JSC::ToIsolateImpl(isolate)->m_group;
}

v8::Local<v8::Context> os_newContext(v8::Isolate *isolate, JSContextGroupRef groupRef,
                                     JSGlobalContextRef *ctxRef)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    *ctxRef = (JSGlobalContextRef) V82JSC::ToContextRef(context);
    return scope.Escape(context);
}

void os_Dispose(JSContextGroupRef group, JSGlobalContextRef ctxRef)
{
    // Nothing to do -- V82JSC will take care of it
}

#else // __ANDROID__

# include "JSC/JSC.h"
# include "JNI/JNI.h"

JSContextGroupRef os_groupFromIsolate(v8::Isolate *isolate, uv_loop_t* event_loop)
{
    return &* OpaqueJSContextGroup::New(isolate, event_loop);
}

v8::Local<v8::Context> os_newContext(v8::Isolate *isolate, JSContextGroupRef groupRef,
                                     JSGlobalContextRef *ctxRef)
{
    EscapableHandleScope scope(isolate);

    JSClassRef globalClass = nullptr;
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.attributes |= kJSClassAttributeNoAutomaticPrototype;
        globalClass = JSClassCreate(&definition);
        *ctxRef = JSGlobalContextCreateInGroup(group, globalClass);
    }
    JSClassRelease(globalClass);
    auto java_node_context = (*ctxRef)->Context();
    Local<Context> context = java_node_context->Value();
    return scope.Escape(context);
}

void os_Dispose(JSContextGroupRef group, JSGlobalContextRef ctxRef)
{
    JSGlobalContextRelease(ctxRef);
    JSContextGroupRelease(group);
    group->Dispose();
}

#endif
