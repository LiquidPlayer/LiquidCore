/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <boost/make_shared.hpp>
#include <algorithm>
#include "JavaScriptCore/JavaScript.h"
#include "JSC/OpaqueJSContextGroup.h"

boost::shared_ptr<OpaqueJSContextGroup> OpaqueJSContextGroup::New()
{
    auto group = boost::make_shared<OpaqueJSContextGroup>();
    group->m_self = group;
    return group;
}

boost::shared_ptr<OpaqueJSContextGroup> OpaqueJSContextGroup::New(Isolate *isolate,
    uv_loop_t *event_loop)
{
    auto group = boost::make_shared<OpaqueJSContextGroup>(isolate, event_loop);
    group->m_self = group;
    return group;
}

OpaqueJSContextGroup::OpaqueJSContextGroup(Isolate *isolate, uv_loop_t *event_loop) :
    ContextGroup(isolate, event_loop), m_jsc_count(1)
{
}

OpaqueJSContextGroup::OpaqueJSContextGroup() : ContextGroup(), m_jsc_count(1)
{
}

OpaqueJSContextGroup::~OpaqueJSContextGroup()
{
}

void OpaqueJSContextGroup::AssociateContext(const OpaqueJSContext* ctx)
{
    m_mutex.lock();
    m_associatedContexts.push_back(ctx);
    m_mutex.unlock();
}

void OpaqueJSContextGroup::DisassociateContext(const OpaqueJSContext* ctx)
{
    m_mutex.lock();
    auto it = std::find(m_associatedContexts.begin(), m_associatedContexts.end(), ctx);
    if(it != m_associatedContexts.end())
        m_associatedContexts.erase(it);
    m_mutex.unlock();
}

void OpaqueJSContextGroup::Retain()
{
    m_jsc_count ++;
}

void OpaqueJSContextGroup::Release()
{
    if (--m_jsc_count == 0) {
        m_mutex.lock();
        while (!m_associatedContexts.empty()) {
            JSGlobalContextRef ctx = const_cast<JSGlobalContextRef>(&*m_associatedContexts.front());
            m_mutex.unlock();
            JSGlobalContextRelease(ctx);
            m_mutex.lock();
        }
        m_mutex.unlock();
        {
            boost::shared_ptr<ContextGroup> self = m_self;
            self.reset();
        }
    }
}
