/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
*/
#include "Common/JSContext.h"
#include "Common/JSValue.h"
#include "Common/Macros.h"
#include <boost/make_shared.hpp>

boost::shared_ptr<JSContext> JSContext::New(boost::shared_ptr<ContextGroup> isolate, Local<Context> val)
{
    auto p = boost::make_shared<JSContext>(isolate, val);
    isolate->Manage(p);
    return p;
}

JSContext::JSContext(boost::shared_ptr<ContextGroup> isolate, Local<Context> val) {
    m_isolate = isolate;
    m_context.Reset(isolate->isolate(), val);
    m_isDefunct = false;
}

JSContext::~JSContext() {
    Dispose();
};

void JSContext::Dispose() {
    if (!m_isDefunct) {
        m_isDefunct = true;

        m_set_mutex.lock();
        for (auto it = m_value_set.begin(); it != m_value_set.end(); ++it) {
            auto p = boost::atomic_load<JSValue>(&(*it));
            p.reset();
        }
        m_value_set.clear();
        m_set_mutex.unlock();

        m_context.Reset();
        {
            boost::shared_ptr<ContextGroup> isolate = m_isolate;
            isolate.reset();
        }
    }
}

void JSContext::retain(boost::shared_ptr<JSValue> value) {
    m_set_mutex.lock();
    m_value_set.push_back(value);
    m_set_mutex.unlock();
}

boost::shared_ptr<JSValue> JSContext::Global() {
    Local<v8::Value> global = Value()->Global();
    boost::shared_ptr<JSContext> ctx = shared_from_this();
    return JSValue::New(ctx, global);
}
