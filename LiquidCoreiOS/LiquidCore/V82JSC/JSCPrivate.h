/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef JSCPrivate_hpp
#define JSCPrivate_hpp

#include <JavaScriptCore/JavaScript.h>
#include "v8.h"

namespace v8 { namespace internal {
struct IsolateImpl; }}

namespace JSCPrivate {
    void * LockJSC(v8::internal::IsolateImpl* iso, JSContextGroupRef g);
    v8::internal::IsolateImpl* UnlockJSC(void* token);
    
    void * UnlockAllJSCLocks(v8::internal::IsolateImpl* iso, JSContextGroupRef g);
    v8::internal::IsolateImpl* ReinstateAllJSCLocks(void *token);
    
    bool HasLock(v8::internal::IsolateImpl* iso, JSContextGroupRef g);
    
    void PrintLockOwner(v8::internal::IsolateImpl* iso, JSContextGroupRef g);
    void PrintCurrentThread();
    unsigned GetLockOwner(v8::internal::IsolateImpl* iso, JSContextGroupRef g);
    
    struct JSMarker;
    typedef struct JSMarker JSMarker;
    typedef JSMarker *JSMarkerRef;
    struct JSMarker {
        bool (*IsMarked)(JSMarkerRef, JSObjectRef);
        void (*Mark)(JSMarkerRef, JSObjectRef);
    };
    typedef void (*JSMarkingConstraint)(JSMarkerRef, void *userData);
    typedef void (*JSHeapFinalizer)(JSContextGroupRef, void *userData);
    typedef bool (*JSShouldTerminateCallback) (JSContextRef ctx, void* context);

    typedef const void* JSWeakRef;
    typedef void* JSScriptRef;

    void JSScriptRelease(JSScriptRef script);
    JSValueRef JSScriptEvaluate(JSContextRef ctx, JSScriptRef script, JSValueRef thisValue, JSValueRef* exception);
    JSScriptRef JSScriptCreateFromString(JSContextGroupRef contextGroup, JSContextRef ctx, JSStringRef url,
                                         int startingLineNumber, JSStringRef source, JSStringRef* errorMessage, int* errorLine);
    JSStringRef JSContextCreateBacktrace(JSContextRef ctx, unsigned maxStackSize);
    JSWeakRef JSWeakCreate(JSContextGroupRef, JSObjectRef);
    void JSWeakRelease(JSContextGroupRef, JSWeakRef);
    JSObjectRef JSWeakGetObject(JSWeakRef);
    void JSContextGroupSetExecutionTimeLimit(JSContextGroupRef group, double limit, JSShouldTerminateCallback callback, void* context);
    void JSContextGroupClearExecutionTimeLimit(JSContextGroupRef group);
    JSGlobalContextRef JSObjectGetGlobalContext(JSObjectRef object);
    JSObjectRef JSObjectGetProxyTarget(v8::Local<v8::Context>,JSObjectRef);
    void JSContextGroupAddMarkingConstraint(JSContextGroupRef, JSMarkingConstraint, void *userData);
    void JSContextGroupAddHeapFinalizer(JSContextGroupRef, JSHeapFinalizer, v8::internal::IsolateImpl *userData);
    void JSContextGroupRemoveHeapFinalizer(JSContextGroupRef, JSHeapFinalizer, v8::internal::IsolateImpl *userData);
    void JSSynchronousGarbageCollectForDebugging(JSContextRef);
};



#endif /* JSCPrivate_hpp */
