//
// OpaqueJSContext.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

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
#include <algorithm>
#include "JSC/OpaqueJSContextGroup.h"
#include "JSC/OpaqueJSContext.h"
#include "JSC/OpaqueJSValue.h"

JSGlobalContextRef OpaqueJSContext::New(std::shared_ptr<JSContext> ctx)
{
    JSGlobalContextRef p = new OpaqueJSContext(ctx);
    static_cast<OpaqueJSContextGroup *>(&* ctx->Group())->AssociateContext(p);
    return p;
}

void OpaqueJSContext::StaticGCCallback(GCType type, GCCallbackFlags flags, void*data)
{
    reinterpret_cast<OpaqueJSContext*>(data)->GCCallback(type,flags);
}

OpaqueJSContext::OpaqueJSContext(std::shared_ptr<JSContext> ctx) : m_context(ctx), m_isDefunct(false)
{
    ctx->Group()->RegisterGCCallback(StaticGCCallback, this);
}

OpaqueJSContext::~OpaqueJSContext()
{
    Dispose();
}

void OpaqueJSContext::Dispose()
{
    if (!m_isDefunct) {
        m_isDefunct = true;
        static_cast<OpaqueJSContextGroup *>(&*Context()->Group())
            ->DisassociateContext(this);

        V8_ISOLATE(m_context->Group(), isolate);
            m_context->Group()->UnregisterGCCallback(StaticGCCallback, this);

            ForceGC();
            //For testing only.  Must also specify --enable_gc flag in common.cpp
            //isolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);

            m_gc_lock.lock();

            // First, look for all values that have a zero reference and clean them
            auto iterator = m_collection.begin();
            while (iterator != m_collection.end()) {
                const auto& v = *iterator;
                ++iterator;
                v->Clean(true);
            }

            // Then, release everything that has a reference count > 0
            bool isEmpty =  m_collection.empty();
            while (!isEmpty) {
                const auto& v = m_collection.front();
                const_cast<OpaqueJSValue *>(v)->Release();
                isEmpty =  m_collection.empty();
            }
            m_gc_lock.unlock();

            ASSERTJSC(m_collection.empty());

            m_context.reset();
        V8_UNLOCK();
    }
}

bool OpaqueJSContext::IsDefunct()
{
    return m_isDefunct;
}

void OpaqueJSContext::MarkForCollection(JSValueRef value)
{
    ASSERTJSC(value->Context() == this);
    m_gc_lock.lock();
    m_collection.push_back(value);
    m_gc_lock.unlock();
}

void OpaqueJSContext::MarkCollected(JSValueRef value)
{
    ASSERTJSC(value->Context() == this);
    m_gc_lock.lock();
    auto it = std::find(m_collection.begin(), m_collection.end(), value);
    if(it != m_collection.end())
        m_collection.erase(it);
    m_gc_lock.unlock();
}

void OpaqueJSContext::GCCallback(GCType type, GCCallbackFlags flags)
{
}

void OpaqueJSContext::ForceGC()
{
    V8_ISOLATE(Context()->Group(), isolate)
        while(!isolate->IdleNotificationDeadline(
            group_->Platform()->MonotonicallyIncreasingTime() + 1.0)) {};
    V8_UNLOCK()
}
