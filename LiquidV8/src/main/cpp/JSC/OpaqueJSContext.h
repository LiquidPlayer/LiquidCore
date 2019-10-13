/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_OPAQUEJSCONTEXT_H_H
#define LIQUIDCORE_OPAQUEJSCONTEXT_H_H

#include "JavaScriptCore/JavaScript.h"
#include "JSC/JSCRetainer.h"

struct OpaqueJSContext : public JSCRetainer {
    public:
        static JSGlobalContextRef New(boost::shared_ptr<JSContext> ctx);
        virtual ~OpaqueJSContext();

        inline boost::shared_ptr<JSContext> Context() const { return m_context; }
        void MarkForCollection(JSValueRef value);
        void MarkCollected(JSValueRef value);
        void ForceGC();
        void Dispose();
        bool IsDefunct();

    private:
        OpaqueJSContext(boost::shared_ptr<JSContext> ctx);
        void GCCallback(GCType type, GCCallbackFlags flags);

        boost::atomic_shared_ptr<JSContext> m_context;
        std::vector<JSValueRef> m_collection;
        std::recursive_mutex m_gc_lock;
        bool m_isDefunct;

        static void StaticGCCallback(GCType type, GCCallbackFlags flags, void*data);
};

#endif //LIQUIDCORE_OPAQUEJSCONTEXT_H_H
