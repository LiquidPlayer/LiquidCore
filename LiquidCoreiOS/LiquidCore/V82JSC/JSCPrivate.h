/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef JSCPrivate_hpp
#define JSCPrivate_hpp

#include <JavaScriptCore/JavaScript.h>
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
};

#endif /* JSCPrivate_hpp */
