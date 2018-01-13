//
// OpaqueJSContextGroup.cpp
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
