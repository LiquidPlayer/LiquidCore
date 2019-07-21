/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <algorithm>
#include "JSC/OpaqueJSContextGroup.h"
#include "JSC/OpaqueJSContext.h"
#include "JSC/OpaqueJSValue.h"

JSGlobalContextRef OpaqueJSContext::New(boost::shared_ptr<JSContext> ctx)
{
    JSGlobalContextRef p = new OpaqueJSContext(ctx);
    static_cast<OpaqueJSContextGroup *>(&* ctx->Group())->AssociateContext(p);
    return p;
}

void OpaqueJSContext::StaticGCCallback(GCType type, GCCallbackFlags flags, void*data)
{
    reinterpret_cast<OpaqueJSContext*>(data)->GCCallback(type,flags);
}

OpaqueJSContext::OpaqueJSContext(boost::shared_ptr<JSContext> ctx) : m_context(ctx), m_isDefunct(false)
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

        boost::shared_ptr<JSContext> context = m_context;
        V8_ISOLATE(context->Group(), isolate);
            context->Group()->UnregisterGCCallback(StaticGCCallback, this);

            ForceGC();
            //For testing only.  Must also specify --enable_gc flag in common.cpp
            //isolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);

            m_gc_lock.lock();

            // First, look for all values that have a zero reference and clean them
            // Then, release everything that has a reference count > 0
            bool isEmpty = m_collection.empty();
            while (!isEmpty) {
                const auto& v = m_collection.front();
                const_cast<OpaqueJSValue*>(v)->ClearWeak();
                if (v->IsDefunct()) {
                    v->Clean();
                } else {
                    const_cast<OpaqueJSValue*>(v)->Release();
                }
                isEmpty = m_collection.empty();
            }
            m_gc_lock.unlock();

            context.reset();
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
