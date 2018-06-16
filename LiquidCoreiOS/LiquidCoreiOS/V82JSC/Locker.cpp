//
//  Locker.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Unlocker::~Unlocker()
{
    IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(isolate_);
    if (iso->m_locker) {
        iso->m_locker->lock();
    }
}

void Unlocker::Initialize(Isolate* isolate)
{
    isolate_ = reinterpret_cast<internal::Isolate*>(isolate);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    if (iso->m_locker) {
        iso->m_locker->unlock();
    }
}


/**
 * Returns whether or not the locker for a given isolate, is locked by the
 * current thread.
 */
bool Locker::IsLocked(Isolate* isolate)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    if (iso->m_locker==nullptr) return false;
    bool have_locked = iso->m_locker->try_lock();
    if (have_locked) {
        bool locked_by_this_thread = iso->m_isLocked;
        iso->m_locker->unlock();
        return locked_by_this_thread;
    }
    return false;
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
    isolate_ = reinterpret_cast<internal::Isolate*>(isolate);
    has_lock_ = false;
    top_level_ = false;
    
    IsolateImpl *iso = V82JSC::ToIsolateImpl(isolate);
    if (!iso->m_locker) {
        iso->m_locker = new std::recursive_mutex();
    }
    iso->m_locker->lock();
    has_lock_ = iso->m_isLocked;
    iso->m_isLocked = true;
    IsolateImpl::s_isLockerActive = true;
}

Locker::~Locker()
{
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(isolate_);
    if (iso->m_locker) {
        iso->m_isLocked = has_lock_;
        iso->m_locker->unlock();
    }
}
