//
// LoopPreserver.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
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
#include <boost/make_shared.hpp>
#include "Common/LoopPreserver.h"
#include "Common/ContextGroup.h"

boost::shared_ptr<LoopPreserver> LoopPreserver::New(boost::shared_ptr<ContextGroup> group)
{
    auto preserver = boost::make_shared<LoopPreserver>(group);
    return preserver;
}

LoopPreserver::LoopPreserver(boost::shared_ptr<ContextGroup> group) :
        m_isDefunct(false), m_group(group), m_javaReference(0)
{
    auto done = [](uv_async_t* handle) {
        uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
            delete (uv_async_t*)h;
        });
    };

    m_async_handle = new uv_async_t();
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
