/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
*/

#ifndef LIQUIDCORE_LOOPPRESERVER_H
#define LIQUIDCORE_LOOPPRESERVER_H

#include <memory>
#include <jni.h>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/atomic.hpp>
#include "uv.h"

class ContextGroup;

class LoopPreserver : public boost::enable_shared_from_this<LoopPreserver>
{
public:
    static boost::shared_ptr<LoopPreserver> New(boost::shared_ptr<ContextGroup> group);
    LoopPreserver(boost::shared_ptr<ContextGroup> group);
    virtual ~LoopPreserver();
    void Dispose();
    inline bool IsDefunct() { return m_isDefunct; }
    inline boost::shared_ptr<ContextGroup> Group() { return m_group; }

private:
    bool m_isDefunct;
    uv_async_t * m_async_handle;
    boost::atomic_shared_ptr<ContextGroup> m_group;
};

#endif //LIQUIDCORE_LOOPPRESERVER_H
