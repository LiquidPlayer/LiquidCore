//
//  JSCPrivate.cpp
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
