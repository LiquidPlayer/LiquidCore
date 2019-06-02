/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSCPrivate.h"

/*
 * To use the JSC internal lock implementation, uncomment the next line and include
 * the following directories in the SYSTEM header path:
 *
 * $(WebKit)/Source
 * $(WebKit)/Source/WTF
 * $(WebKit)/Source/JavaScriptCore/heap
 * $(WebKit)/Source/JavaScriptCore/runtime
 * $(WebKit)/Source/JavaScriptCore/jit
 * $(WebKit)/Source/JavaScriptCore/assembler
 * $(WebKit)/Source/JavaScriptCore/bytecode
 * $(WebKit)/Source/JavaScriptCore/interpreter
 * $(WebKit)/Source/JavaScriptCore/wasm
 */
// #define USE_JAVASCRIPTCORE_INTERNALS

#if defined(USE_JAVASCRIPTCORE_INTERNALS)

#define JS_EXPORT_PRIVATE
#define WTF_MAKE_FAST_ALLOCATED
#define USE_SYSTEM_MALLOC 1
#define HAVE_FAST_TLS 1
#include <JavaScriptCore/JavaScriptCore.h>
#include <JavaScriptCore/runtime/JSLock.h>
#include <JavaScriptCore/runtime/VM.h>

#else
#include "V82JSC.h"
#endif

using v8::internal::IsolateImpl;

#if defined(USE_JAVASCRIPTCORE_INTERNALS)

static inline JSC::VM* toJS(JSContextGroupRef g)
{
    return reinterpret_cast<JSC::VM*>(const_cast<OpaqueJSContextGroup*>(g));
}

class DropLocks : public JSC::JSLock::DropAllLocks
{
public:
    DropLocks(IsolateImpl* iso, JSContextGroupRef g) : DropAllLocks(toJS(g)), iso_(iso), group_(g) {}
    IsolateImpl* iso_;
    JSContextGroupRef group_;
};

void * JSCPrivate::UnlockAllJSCLocks(IsolateImpl* iso, JSContextGroupRef g)
{
    auto token = new DropLocks(iso,g);
    auto thread = & WTF::Thread::current();
    unsigned owner = GetLockOwner(g);
    printf("UNLOCKER UNLOCK[%x] depth = %d, owner = %x\n", thread->machThread(), token->dropDepth(), owner);
    return token;
}

IsolateImpl* JSCPrivate::ReinstateAllJSCLocks(void *token)
{
    auto dal = (DropLocks*) token;
    auto thread = & WTF::Thread::current();
    unsigned owner = GetLockOwner(dal->group_);
    printf("UNLOCKER RELOCK[%x] depth = %d, owner = %x\n", thread->machThread(), dal->dropDepth(), owner);
    IsolateImpl *iso = dal->iso_;
    delete dal;
    printf("RELOCKED.\n");
    return iso;
}

class LockIsolate : public JSC::JSLockHolder {
public:
    LockIsolate(IsolateImpl *iso, JSContextGroupRef g) : JSLockHolder(toJS(g)), iso_(iso), group_(g) {}
    IsolateImpl *iso_;
    JSContextGroupRef group_;
};

void * JSCPrivate::LockJSC(IsolateImpl* iso, JSContextGroupRef g)
{
    auto token = new LockIsolate(iso, g);
    auto thread = & WTF::Thread::current();
    unsigned owner = GetLockOwner(g);
    printf("LOCKER LOCK[%x], owner = %x\n", thread->machThread(), owner);
    return token;
}

IsolateImpl* JSCPrivate::UnlockJSC(void* token)
{
    auto holder = (LockIsolate*)token;
    auto thread = & WTF::Thread::current();
    unsigned owner = GetLockOwner(holder->group_);
    printf("LOCKER UNLOCK[%x], owner = %x\n", thread->machThread(), owner);
    IsolateImpl *iso = holder->iso_;
    return iso;
}

bool JSCPrivate::HasLock(IsolateImpl* iso, JSContextGroupRef g)
{
    JSC::VM* vm = toJS(g);
    return vm->apiLock().currentThreadIsHoldingLock();
}

unsigned JSCPrivate::GetLockOwner(IsolateImpl* iso, JSContextGroupRef g)
{
    JSC::VM* vm = toJS(g);
    auto owner = vm->apiLock().ownerThread();
    if (owner == std::nullopt) {
        return 0;
    } else {
        return owner->get()->machThread();
    }
}

void JSCPrivate::PrintLockOwner(IsolateImpl* iso, JSContextGroupRef g)
{
    unsigned owner = GetLockOwner(iso, g);
    if (owner == 0) {
        printf ("Owner: NONE\n");
    } else {
        printf ("Owner: %x\n", owner);
    }
}

void JSCPrivate::PrintCurrentThread()
{
    auto thread = & WTF::Thread::current();
    printf ("Current: %x\n", thread->machThread());
}

#else // USE_JAVASCRIPTCORE_INTERNALS

void * JSCPrivate::LockJSC(IsolateImpl* iso, JSContextGroupRef g)
{
    if (iso->m_locker == nullptr) {
        iso->m_locker = new std::recursive_mutex();
        iso->m_locks = 0;
    }
    iso->m_locker->lock();
    iso->m_locks ++;
    iso->m_owner = std::this_thread::get_id();
    return iso;
}

IsolateImpl* JSCPrivate::UnlockJSC(void* token)
{
    IsolateImpl* iso = (IsolateImpl*) token;
    int locks = -- iso->m_locks;
    if (locks == 0) iso->m_owner = std::thread::id();
    iso->m_locker->unlock();
    return iso;
}

class DropLocks {
public:
    IsolateImpl* iso_;
    int depth_;
};

void * JSCPrivate::UnlockAllJSCLocks(IsolateImpl* iso, JSContextGroupRef g)
{
    auto drop = new DropLocks();
    drop->iso_ = iso;
    if (!HasLock(iso,g)) {
        drop->depth_ = 0;
    } else {
        drop->depth_ = iso->m_locks;
        iso->m_owner = std::thread::id();
        int locks = iso->m_locks;
        for (int i=0; i<locks; i++) {
            iso->m_locks --;
            iso->m_locker->unlock();
        }
    }
    return drop;
}

IsolateImpl* JSCPrivate::ReinstateAllJSCLocks(void *token)
{
    DropLocks *drop = (DropLocks*)token;
    
    for (int i=0; i<drop->depth_; i++) {
        drop->iso_->m_locker->lock();
        drop->iso_->m_locks ++;
    }
    IsolateImpl* iso = drop->iso_;
    iso->m_owner = std::this_thread::get_id();
    delete drop;
    return iso;
}

bool JSCPrivate::HasLock(IsolateImpl* iso, JSContextGroupRef g)
{
    if (iso->m_locker==nullptr) return false;
    bool have_locked = iso->m_locker->try_lock();
    if (have_locked) {
        bool locked_by_this_thread = (iso->m_locks != 0);
        iso->m_locker->unlock();
        return locked_by_this_thread;
    }
    return false;
}

unsigned JSCPrivate::GetLockOwner(IsolateImpl* iso, JSContextGroupRef g)
{
    size_t hash = std::hash<std::thread::id>()(iso->m_owner);
    return (unsigned) hash;
}

void JSCPrivate::PrintLockOwner(IsolateImpl* iso, JSContextGroupRef g)
{
    unsigned owner = GetLockOwner(iso, g);
    printf ("Owner: %x\n", owner);
}

void JSCPrivate::PrintCurrentThread()
{
    size_t hash = std::hash<std::thread::id>()(std::this_thread::get_id());
    printf ("Current: %x\n", (unsigned)hash);
}

#endif

#ifdef USE_JAVASCRIPTCORE_PRIVATE_API

#include "JSContextRefPrivate.h"
#include "JSObjectRefPrivate.h"
#include "JSScriptRefPrivate.h"
#include "JSWeakRefPrivate.h"
#include "JSHeapFinalizerPrivate.h"
#include "JSMarkingConstraintPrivate.h"
#include "JSStringRefPrivate.h"
extern "C" void JSSynchronousGarbageCollectForDebugging(JSContextRef ctx);

void JSCPrivate::JSScriptRelease(JSScriptRef script)
{
    return ::JSScriptRelease((::JSScriptRef)script);
}

JSValueRef JSCPrivate::JSScriptEvaluate(JSContextRef ctx, JSScriptRef script, JSValueRef thisValue, JSValueRef* exception)
{
    return ::JSScriptEvaluate(ctx, (::JSScriptRef)script, thisValue, exception);
}

JSCPrivate::JSScriptRef JSCPrivate::JSScriptCreateFromString(JSContextGroupRef contextGroup, JSContextRef ctx, JSStringRef url, int startingLineNumber,
                                                             JSStringRef source, JSStringRef* errorMessage, int* errorLine)
{
    return ::JSScriptCreateFromString(contextGroup, url, startingLineNumber, source, errorMessage, errorLine);
}

JSStringRef JSCPrivate::JSContextCreateBacktrace(JSContextRef ctx, unsigned maxStackSize)
{
    return ::JSContextCreateBacktrace(ctx, maxStackSize);
}

JSCPrivate::JSWeakRef JSCPrivate::JSWeakCreate(JSContextGroupRef group, JSObjectRef object)
{
    return reinterpret_cast<JSWeakRef>(::JSWeakCreate(group, object));
}

void JSCPrivate::JSWeakRelease(JSContextGroupRef group, JSWeakRef weak)
{
    return ::JSWeakRelease(group, (::JSWeakRef)weak);
}

JSObjectRef JSCPrivate::JSWeakGetObject(JSWeakRef weak)
{
    return ::JSWeakGetObject((::JSWeakRef)weak);
}

void JSCPrivate::JSContextGroupSetExecutionTimeLimit(JSContextGroupRef group, double limit, JSShouldTerminateCallback callback, void* context)
{
    return ::JSContextGroupSetExecutionTimeLimit(group, limit, callback, context);
}

void JSCPrivate::JSContextGroupClearExecutionTimeLimit(JSContextGroupRef group)
{
    return ::JSContextGroupClearExecutionTimeLimit(group);
}

JSGlobalContextRef JSCPrivate::JSObjectGetGlobalContext(JSObjectRef object)
{
    return ::JSObjectGetGlobalContext(object);
}

JSObjectRef JSCPrivate::JSObjectGetProxyTarget(v8::Local<v8::Context> context, JSObjectRef object)
{
    return ::JSObjectGetProxyTarget(object);
}

void JSCPrivate::JSContextGroupAddMarkingConstraint(JSContextGroupRef group, JSMarkingConstraint constraint, void *userData)
{
    return ::JSContextGroupAddMarkingConstraint(group, (::JSMarkingConstraint)constraint, userData);
}

void JSCPrivate::JSContextGroupAddHeapFinalizer(JSContextGroupRef group, JSHeapFinalizer finalizer, IsolateImpl *userData)
{
    return ::JSContextGroupAddHeapFinalizer(group, finalizer, userData);
}
 
void JSCPrivate::JSContextGroupRemoveHeapFinalizer(JSContextGroupRef group, JSHeapFinalizer finalizer, IsolateImpl *userData)
{
    return ::JSContextGroupRemoveHeapFinalizer(group, finalizer, userData);
}

void JSCPrivate::JSSynchronousGarbageCollectForDebugging(JSContextRef ctx)
{
    ::JSSynchronousGarbageCollectForDebugging(ctx);
}

#else

void JSCPrivate::JSScriptRelease(JSScriptRef script)
{
    // Do nothing
}

JSValueRef JSCPrivate::JSScriptEvaluate(JSContextRef ctx, JSScriptRef script, JSValueRef thisValue, JSValueRef* exception)
{
    // Should not reach
    CHECK(false);
}

JSCPrivate::JSScriptRef JSCPrivate::JSScriptCreateFromString(JSContextGroupRef contextGroup,
                                                             JSContextRef ctx,
                                                             JSStringRef url,
                                                             int startingLineNumber,
                                                             JSStringRef source,
                                                             JSStringRef* errorMessage,
                                                             int* errorLine)
{
    JSValueRef exception = nullptr;
    
    auto success = JSCheckScriptSyntax(ctx, source, url, startingLineNumber, &exception);
    if (!success && exception && errorMessage) {
        *errorMessage = JSValueToStringCopy(ctx, exception, nullptr);
        if (errorLine) *errorLine = 0; // FIXME
    }
    if (!success) return nullptr;
    
    // All we are doing is returning a non-null value.  In this case, we can't use
    // private APIs, so we won't actually use this value.  It just needs to not be null.
    return (JSCPrivate::JSScriptRef) source;
}

JSStringRef JSCPrivate::JSContextCreateBacktrace(JSContextRef ctx, unsigned maxStackSize)
{
    auto trace = V82JSC::exec(ctx, "return (new Error()).stack", 0, 0);
    return JSValueToStringCopy(ctx, trace, nullptr);
}

JSCPrivate::JSWeakRef JSCPrivate::JSWeakCreate(JSContextGroupRef group, JSObjectRef object)
{
    return reinterpret_cast<JSWeakRef>(object);
}

void JSCPrivate::JSWeakRelease(JSContextGroupRef group, JSWeakRef weak)
{
    // Do nothing
}

JSObjectRef JSCPrivate::JSWeakGetObject(JSWeakRef weak)
{
    return (JSObjectRef)weak;
}

void JSCPrivate::JSContextGroupSetExecutionTimeLimit(JSContextGroupRef group, double limit, JSShouldTerminateCallback callback, void* context)
{
    // Do nothing
}

void JSCPrivate::JSContextGroupClearExecutionTimeLimit(JSContextGroupRef group)
{
    // Do nothing
}

JSObjectRef JSCPrivate::JSObjectGetProxyTarget(v8::Local<v8::Context> context, JSObjectRef object)
{
    v8::HandleScope scope(V82JSC::ToIsolate(V82JSC::ToContextImpl(context)));
    
    auto gCtx = V82JSC::ToGlobalContextImpl(V82JSC::FindGlobalContext(context));

    JSValueRef args[] = {
        gCtx->m_proxy_targets,
        object
    };
    
    return (JSObjectRef) V82JSC::exec(V82JSC::ToContextRef(context),
                                      "return _1.get(_2)",
                                      2, args);
}

void JSCPrivate::JSContextGroupAddMarkingConstraint(JSContextGroupRef group, JSMarkingConstraint constraint, void *userData)
{
    // Do nothing.  There will not be a chance for first pass callbacks to attempt to save marked
    // objects.
}


void JSCPrivate::JSContextGroupAddHeapFinalizer(JSContextGroupRef group, JSHeapFinalizer finalizer, IsolateImpl *userData)
{
    // This is a stupid hack.  We will just trigger the finalizer every 5 seconds.
    userData->m_kill_collection_thread = false;
    auto collectionThread = new std::thread([finalizer,userData](){
        while (!userData->m_kill_collection_thread) {
            sleep(5);
            if (!userData->m_kill_collection_thread) {
                finalizer(userData->m_group, userData);
            }
        }
    });
}
 
void JSCPrivate::JSContextGroupRemoveHeapFinalizer(JSContextGroupRef group, JSHeapFinalizer finalizer, IsolateImpl *userData)
{
    userData->m_kill_collection_thread = true;
    userData->m_collection_thread->join();
    delete userData->m_collection_thread;
}

void JSCPrivate::JSSynchronousGarbageCollectForDebugging(JSContextRef ctx)
{
    // Ignore
}

JSGlobalContextRef JSCPrivate::JSObjectGetGlobalContext(JSObjectRef object)
{
    // Should not reach
    CHECK(false);
}

#endif
