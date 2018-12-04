/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
        *ctxRef = JSGlobalContextCreateInGroup(groupRef, globalClass);
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
    const_cast<OpaqueJSContextGroup*>(group)->Dispose();
}

#endif
