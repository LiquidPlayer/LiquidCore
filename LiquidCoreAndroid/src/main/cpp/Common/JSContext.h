/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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

    inline Local<Context> Value() {
        EscapableHandleScope scope(Isolate::GetCurrent());
        return scope.Escape(Local<Context>::New(isolate(), m_context));
    }
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
