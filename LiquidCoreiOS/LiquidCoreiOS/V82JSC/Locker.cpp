//
//  Locker.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSCPrivate.h"

using namespace v8;

void Unlocker::Initialize(Isolate* isolate)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    isolate_ = (internal::Isolate*) JSCPrivate::UnlockAllJSCLocks(iso, iso->m_group);
}

Unlocker::~Unlocker()
{
    IsolateImpl* iso = JSCPrivate::ReinstateAllJSCLocks(isolate_);
    IsolateImpl::PerThreadData::EnterThreadContext(reinterpret_cast<v8::Isolate*>(iso));
}

/**
 * Returns whether or not the locker for a given isolate, is locked by the
 * current thread.
 */
bool Locker::IsLocked(Isolate* isolate)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    return JSCPrivate::HasLock(iso, iso->m_group);
}

/**
 * Returns whether v8::Locker is being used by this V8 instance.
 */
bool Locker::IsActive()
{
    return IsolateImpl::s_isLockerActive;
}

void Locker::Initialize(Isolate* isolate)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(isolate);
    isolate_ = (internal::Isolate*) JSCPrivate::LockJSC(iso, iso->m_group);
    IsolateImpl::s_isLockerActive = true;
}

Locker::~Locker()
{
    JSCPrivate::UnlockJSC(isolate_);
}
