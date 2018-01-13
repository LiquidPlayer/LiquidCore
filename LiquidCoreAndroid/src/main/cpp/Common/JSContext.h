//
// JSContext.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 - 2018 Eric Lange. All rights reserved.

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
#ifndef LIQUIDCORE_JSCONTEXT_H
#define LIQUIDCORE_JSCONTEXT_H

#include "Common/ContextGroup.h"

using namespace v8;

class JSValue;

class JSContext : public boost::enable_shared_from_this<JSContext> {
public:
    static boost::shared_ptr<JSContext> New(boost::shared_ptr<ContextGroup> isolate, Local<Context> val);
    virtual ~JSContext();
    JSContext(boost::shared_ptr<ContextGroup> isolate, Local<Context> val);

    boost::shared_ptr<JSValue>      Global();

    inline Local<Context> Value() { return Local<Context>::New(isolate(), m_context); }
    inline Isolate* isolate() {
        boost::shared_ptr<ContextGroup> sp = m_isolate;
        return sp->isolate();
    }
    inline boost::shared_ptr<ContextGroup> Group() {
        boost::shared_ptr<ContextGroup> sp = m_isolate;
        return sp;
    }

    void Dispose();
    inline bool IsDefunct() { return m_isDefunct; }

    void retain(boost::shared_ptr<JSValue>);

private:
    Persistent<Context, CopyablePersistentTraits<Context>> m_context;
    boost::atomic_shared_ptr<ContextGroup> m_isolate;
    bool m_isDefunct;
    std::vector<boost::shared_ptr<JSValue>> m_value_set;
    std::recursive_mutex m_set_mutex;
};

#endif //LIQUIDCORE_JSCONTEXT_H
