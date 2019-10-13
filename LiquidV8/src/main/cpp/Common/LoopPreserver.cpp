/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
*/
#include <boost/make_shared.hpp>
#include "Common/LoopPreserver.h"
#include "Common/ContextGroup.h"

boost::shared_ptr<LoopPreserver> LoopPreserver::New(boost::shared_ptr<ContextGroup> group)
{
    auto preserver = boost::make_shared<LoopPreserver>(group);
    return preserver;
}

LoopPreserver::LoopPreserver(boost::shared_ptr<ContextGroup> group) :
        m_isDefunct(false), m_group(group)
{
    auto done = [](uv_async_t* handle) {
        uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
            free(h);
        });
    };

    m_async_handle = (uv_async_t*) malloc (sizeof (uv_async_t));
    uv_async_init(group->Loop(), m_async_handle, done);
}

LoopPreserver::~LoopPreserver()
{
    Dispose();
}

void LoopPreserver::Dispose()
{
    if (!m_isDefunct) {
        m_isDefunct = true;

        uv_async_send(m_async_handle);
    }
}
