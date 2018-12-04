/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_OPAQUEJSCONTEXTGROUP_H
#define LIQUIDCORE_OPAQUEJSCONTEXTGROUP_H

#include "JSC/JSCRetainer.h"

struct OpaqueJSContext;

struct OpaqueJSContextGroup : public ContextGroup {
    public:
        static boost::shared_ptr<OpaqueJSContextGroup> New();
        static boost::shared_ptr<OpaqueJSContextGroup> New(Isolate *isolate, uv_loop_t *event_loop);
        OpaqueJSContextGroup(Isolate *isolate, uv_loop_t *event_loop);
        OpaqueJSContextGroup();
        virtual ~OpaqueJSContextGroup();

        void AssociateContext(const OpaqueJSContext* ctx);
        void DisassociateContext(const OpaqueJSContext* ctx);

        void Retain();
        void Release();

    private:
        int m_jsc_count;
        std::vector<const OpaqueJSContext *> m_associatedContexts;
        std::mutex m_mutex;
    protected:
        boost::atomic_shared_ptr<ContextGroup> m_self;
};

#endif //LIQUIDCORE_OPAQUEJSCONTEXTGROUP_H
