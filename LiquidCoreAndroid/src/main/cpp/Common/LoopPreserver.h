//
// LoopPreserver.h
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

#ifndef LIQUIDCORE_LOOPPRESERVER_H
#define LIQUIDCORE_LOOPPRESERVER_H

#include <memory>
#include "uv.h"

class ContextGroup;

class LoopPreserver : public std::enable_shared_from_this<LoopPreserver>
{
public:
    static std::shared_ptr<LoopPreserver> New(std::shared_ptr<ContextGroup> group);
    LoopPreserver(std::shared_ptr<ContextGroup> group);
    virtual ~LoopPreserver();
    void Dispose();
    inline bool IsDefunct() { return m_isDefunct; }
    inline std::shared_ptr<ContextGroup> Group() { return m_group; }

private:
    bool m_isDefunct;
    uv_async_t * m_async_handle;
    std::shared_ptr<ContextGroup> m_group;
};

#endif //LIQUIDCORE_LOOPPRESERVER_H
