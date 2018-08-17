//
//  Locker.cpp
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
