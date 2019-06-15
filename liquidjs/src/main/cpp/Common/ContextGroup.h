/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
*/
#ifndef LIQUIDCORE_CONTEXTGROUP_H
#define LIQUIDCORE_CONTEXTGROUP_H

#include "v8.h"
#include "libplatform/libplatform.h"
#include "uv.h"

#include <jni.h>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <list>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/atomic.hpp>

#define CONTEXT_GARBAGE_COLLECTED_BUT_PROCESS_STILL_ACTIVE 222

using namespace v8;

class GenericAllocator;
class JSValue;
class JSContext;
class LoopPreserver;

class ContextGroup : public boost::enable_shared_from_this<ContextGroup> {
public:
    ContextGroup();
    ContextGroup(Isolate *isolate, uv_loop_t *uv_loop);
    ContextGroup(char *snapshot, int size);
    virtual ~ContextGroup();

    inline Isolate* isolate() { return m_isDefunct ? nullptr : m_isolate; }
    inline uv_loop_t * Loop() { return m_isDefunct ? nullptr : m_uv_loop; }
    inline bool IsDefunct() { return m_isDefunct; }
    inline std::thread::id Thread() { return m_thread_id; }
    inline boost::shared_ptr<ContextGroup> Group() { return shared_from_this(); }
    inline void sync(std::function<void()> runnable)
    {
        if (!Loop() || std::this_thread::get_id() == Thread()) {
            runnable();
        } else {
            sync_(runnable);
        }
    }
    void RegisterGCCallback(void (*cb)(GCType type, GCCallbackFlags flags, void*), void *);
    void UnregisterGCCallback(void (*cb)(GCType type, GCCallbackFlags flags,void*), void *);
    void Manage(boost::shared_ptr<JSValue> obj);
    void Manage(boost::shared_ptr<JSContext> obj);
    void Dispose();
    void MarkZombie(boost::shared_ptr<JSValue> obj);
    void MarkZombie(boost::shared_ptr<JSContext> obj);
    // These are just here for the SharedWrap template
    void MarkZombie(boost::shared_ptr<ContextGroup> obj) {}
    void MarkZombie(boost::shared_ptr<LoopPreserver> obj) {}
    void FreeZombies();

    void schedule_java_runnable(JNIEnv *env, jobject thiz, jobject runnable);

    static void init_v8();
    static void dispose_v8();
    static inline std::mutex *Mutex() { return &s_mutex; }
    static inline std::unique_ptr<v8::Platform>& Platform() { return s_platform; }
    static void callback(uv_async_t* handle);
    static void StaticGCPrologueCallback(Isolate *isolate, GCType type, GCCallbackFlags flags);
    static boost::shared_ptr<ContextGroup> New(const char *snapshotFile);

protected:
    void GCPrologueCallback(GCType type, GCCallbackFlags flags);

private:
    static std::unique_ptr<v8::Platform> s_platform;
    static int s_init_count;
    static std::mutex s_mutex;
    static std::map<Isolate *, ContextGroup *> s_isolate_map;

    void sync_(std::function<void()> runnable);

    Isolate *m_isolate;
    Isolate::CreateParams m_create_params;
    bool m_manage_isolate;
    uv_loop_t *m_uv_loop;
    std::thread::id m_thread_id;
    std::vector<boost::weak_ptr<JSValue>> m_managedValues;
    std::vector<boost::weak_ptr<JSContext>> m_managedContexts;
    std::vector<boost::shared_ptr<JSValue>> m_value_zombies;
    std::vector<boost::shared_ptr<JSContext>> m_context_zombies;
    std::mutex m_zombie_mutex;
    bool m_isDefunct;

    struct GCCallback {
        void (*cb)(GCType type, GCCallbackFlags flags, void*);
        void *data;
    };
    std::list<std::unique_ptr<struct GCCallback>> m_gc_callbacks;

    uv_async_t *m_async_handle;
    std::vector<void *> m_runnables;
    std::mutex m_async_mutex;

    v8::StartupData m_startup_data;
};

#endif //LIQUIDCORE_CONTEXTGROUP_H
