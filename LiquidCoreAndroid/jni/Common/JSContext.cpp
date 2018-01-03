//
// JSContext.cpp
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
#include "Common/JSContext.h"
#include "Common/JSvalue.h"
#include "Common/Macros.h"

bool JSContext::IsDefunct()
{
    return m_isDefunct;
}

std::shared_ptr<JSContext> JSContext::New(std::shared_ptr<ContextGroup> isolate, Local<Context> val)
{
    auto p = std::make_shared<JSContext>(isolate, val);
    isolate->ManageObject(p);
    return p;
}

JSContext::JSContext(std::shared_ptr<ContextGroup> isolate, Local<Context> val) {
    m_isolate = isolate;
    m_context = Persistent<Context,CopyablePersistentTraits<Context>>(isolate->isolate(), val);
    m_isDefunct = false;
}

JSContext::~JSContext() {
    Dispose();

    /*
    V8_ISOLATE(m_isolate, isolate)
        m_context.Reset();
    V8_UNLOCK()
    */
};

void JSContext::Dispose() {
    if (!m_isDefunct) {
        m_isDefunct = true;

        m_set_mutex.lock();
        //m_value_set.clear();
        m_set_mutex.unlock();
    }
}

void JSContext::retain(std::shared_ptr<JSValue> value) {
    m_set_mutex.lock();
    m_value_set.insert(value);
    m_set_mutex.unlock();
}

std::shared_ptr<JSValue> JSContext::Global() {
    Local<v8::Value> global = Value()->Global();
    std::shared_ptr<JSContext> ctx = shared_from_this();
    return JSValue::New(ctx, global);
}

Local<Context> JSContext::Value() {
    return Local<Context>::New(isolate(), m_context);
}

Isolate* JSContext::isolate() {
    return m_isolate->isolate();
}

std::shared_ptr<ContextGroup> JSContext::Group() {
    return m_isolate;
}
