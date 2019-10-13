/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <JavaScriptCore/JavaScript.h>
#include "node_instance.h"
#include "node_bridge.h"
#include "v8.h"
#include "node.h"
#include "libplatform/libplatform.h"
#undef UNREACHABLE
#undef CHECK
#undef CHECK_EQ
#undef CHECK_NE
#undef CHECK_LE
#undef CHECK_LT
#undef CHECK_GE
#undef CHECK_GT
#undef DISALLOW_COPY_AND_ASSIGN
#undef MUST_USE_RESULT
#undef ROUND_UP
#undef STRINGIFY
#include "V82JSC.h"

using v8::EscapableHandleScope;
using v8::Local;
using v8::Context;
using v8::Object;
using v8::Isolate;
using v8::HandleScope;
using v8::Private;
using v8::String;

class iOSInstance : NodeInstance {
public:
    iOSInstance(OnNodeStartedCallback onStart, OnNodeExitCallback onExit, void* data) :
        m_on_start(onStart), m_on_exit(onExit), m_data(data)
    {
        m_async_handle = nullptr;
    }
    
    static std::mutex s_instance_mutex;
    static std::map<std::thread::id, iOSInstance*> s_instances;
    
    static void node_main_task(void *thiz) {
        reinterpret_cast<iOSInstance*>(thiz)->spawnedThread();
    }

    void spawnedThread()
    {
        m_node_main_thread = std::this_thread::get_id();
        {
            std::unique_lock<std::mutex> lock(s_instance_mutex);
            s_instances[m_node_main_thread] = this;
        }

        enum { kMaxArgs = 64 };
        char cmd[60];

        setenv("NODE_PATH", "/home/node_modules", true);
        strcpy(cmd, "node -e global.__nodedroid_onLoad();");

        int argc = 0;
        char *argv[kMaxArgs];

        char *p2 = strtok(cmd, " ");
        while (p2 && argc < kMaxArgs-1)
        {
            argv[argc++] = p2;
            p2 = strtok(0, " ");
        }
        argv[argc] = 0;

        int ret = node::Start(argc, argv);

        {
            std::unique_lock<std::mutex> lock(s_instance_mutex);
            s_instances.erase(std::this_thread::get_id());
        }

        if (m_on_exit) {
            m_on_exit(m_data, ret);
        }
        
        delete this;
    }

    void sync(ProcessThreadCallback callback, void *data)
    {
        if (std::this_thread::get_id() == m_node_main_thread) {
            callback(data);
            return;
        }
        
        struct Runnable *r = new struct Runnable;
        r->signaled = false;
        r->data = data;
        r->isSync = true;
        r->callback = callback;
     
        std::unique_lock<std::mutex> lk(m_async_mutex);
        m_runnables.push_back(r);

        if (!m_async_handle) {
            m_async_handle = new uv_async_t();
            m_async_handle->data = this;
            uv_async_init(m_loop, m_async_handle, iOSInstance::callback);
            uv_async_send(m_async_handle);
        }

        r->cv.wait(lk, [r]() {
            return r->signaled;
        });
        delete r;
    }

    void async(ProcessThreadCallback callback, void *data)
    {
        if (std::this_thread::get_id() == m_node_main_thread) {
            callback(data);
            return;
        }

        struct Runnable *r = new struct Runnable;
        r->signaled = false;
        r->data = data;
        r->isSync = false;
        r->callback = callback;
        
        std::unique_lock<std::mutex> lk(m_async_mutex);
        m_runnables.push_back(r);
        
        if (!m_async_handle) {
            m_async_handle = new uv_async_t();
            m_async_handle->data = this;
            uv_async_init(m_loop, m_async_handle, iOSInstance::callback);
            uv_async_send(m_async_handle);
        }
    }
    
    uv_async_t * keep_alive()
    {
        auto done = [](uv_async_t* handle) {
            uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
                delete (uv_async_t*)h;
            });
        };
        
        uv_async_t * async_handle = new uv_async_t();
        uv_async_init(m_loop, async_handle, done);
        return async_handle;
    }
    
    void let_die(uv_async_t *async_handle)
    {
        uv_async_send(async_handle);
    }
    
    struct Runnable {
        std::condition_variable cv;
        bool signaled;
        ProcessThreadCallback callback;
        bool isSync;
        void * data;
    };
    
    static void callback(uv_async_t* handle)
    {
        iOSInstance* thiz = reinterpret_cast<iOSInstance*>(handle->data);
        
        thiz->m_async_mutex.lock();
        struct Runnable * r = thiz->m_runnables.empty() ? nullptr : thiz->m_runnables.front();
        while (r) {
            thiz->m_async_mutex.unlock();
            r->callback(r->data);
            thiz->m_async_mutex.lock();
            if (r->isSync) {
                r->signaled = true;
                thiz->m_async_mutex.unlock();
                r->cv.notify_one();
                thiz->m_async_mutex.lock();
            }
            thiz->m_runnables.erase(thiz->m_runnables.begin());
            if (!r->isSync) {
                delete r;
            }
            r = thiz->m_runnables.empty() ? nullptr : thiz->m_runnables.front();
        }
        // Close the handle.  We will create a new one if we
        // need another.  This keeps the node process from staying alive
        // indefinitely
        uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
            delete (uv_async_t*)h;
        });
        thiz->m_async_handle = nullptr;
        thiz->m_async_mutex.unlock();
    }
    
    std::mutex m_async_mutex;
    std::vector<Runnable*> m_runnables;
    uv_async_t *m_async_handle;
    OnNodeStartedCallback m_on_start;
    OnNodeExitCallback m_on_exit;
    void * m_data;
    std::thread::id m_node_main_thread;
    uv_loop_t *m_loop;
    std::atomic<bool> m_kill_collection_thread;
    JSCPrivate::JSHeapFinalizer m_finalizer;
    void *m_finalizer_data;
};

static v8::Platform *s_platform = nullptr;
static int init_count = 0;
std::mutex iOSInstance::s_instance_mutex;
std::map<std::thread::id, iOSInstance*> iOSInstance::s_instances;

v8::Local<v8::Context> NodeInstance::NewContext(v8::Isolate *isolate,
        JSContextGroupRef groupRef, JSGlobalContextRef *ctxRef)
{
    EscapableHandleScope scope(isolate);
    Local<Context> context = Context::New(isolate);
    *ctxRef = (JSGlobalContextRef) V82JSC::ToContextRef(context);
    return scope.Escape(context);
}

JSContextGroupRef NodeInstance::GroupFromIsolate(v8::Isolate *isolate, uv_loop_t* event_loop)
{
    {
        std::unique_lock<std::mutex> lock(iOSInstance::s_instance_mutex);
        auto thiz = iOSInstance::s_instances[std::this_thread::get_id()];
        thiz->m_loop = event_loop;
    }

    return V82JSC::ToIsolateImpl(isolate)->m_group;
}

void NodeInstance::NotifyStart(JSContextRef ctxRef, JSContextGroupRef groupRef)
{
    iOSInstance * thiz;
    {
        std::unique_lock<std::mutex> lock(iOSInstance::s_instance_mutex);
        thiz = iOSInstance::s_instances[std::this_thread::get_id()];
    }

    if (thiz->m_on_start) {
        thiz->m_on_start(thiz->m_data, ctxRef, groupRef);
    }
}

void NodeInstance::RunScavenger(v8::Isolate *isolate)
{
    auto iso = reinterpret_cast<IsolateImpl*>(isolate);
    if (!iso->m_pending_collection) {
        iso->m_pending_collection = true;
        iso->CollectGarbage();
        iso->m_pending_collection = false;
    }
}

static void process_static_init()
{
    if (!init_count ++) {
        node::InitializeNode();
    }
}

extern "C" void * process_start(OnNodeStartedCallback onStart, OnNodeExitCallback onExit, void* data)
{
    process_static_init();
    auto instance = new iOSInstance(onStart, onExit, data);
    return reinterpret_cast<void*>(instance);
}

extern "C" void process_run_in_thread(void *token)
{
    auto instance = reinterpret_cast<iOSInstance*>(token);
    instance->spawnedThread();
}

extern "C" void process_set_filesystem(JSContextRef ctx, JSObjectRef fs)
{
    Isolate *isolate = V82JSC::ToIsolate(IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)]);
    HandleScope scope(isolate);
    
    v8::Local<v8::Context> context = V82JSC::LocalContext::New(isolate, ctx);
    Context::Scope context_scope(context);
    
    Local<Object> globalObj = context->Global();
    Local<Object> fsObj = V82JSC::Value::New(V82JSC::ToContextImpl(context), fs).As<Object>();
    
    Local<Private> privateKey = v8::Private::ForApi(isolate,
                                                    String::NewFromUtf8(isolate, "__fs"));
    globalObj->SetPrivate(context, privateKey, fsObj);
}

extern "C" void process_sync(void* token, ProcessThreadCallback runnable, void* data)
{
    iOSInstance *instance = reinterpret_cast<iOSInstance*>(token);
    instance->sync(runnable, data);
}

extern "C" void process_async(void * token, ProcessThreadCallback runnable, void* data)
{
    iOSInstance *instance = reinterpret_cast<iOSInstance*>(token);
    instance->async(runnable, data);
}

extern "C" void * process_keep_alive(void * token)
{
    iOSInstance *instance = reinterpret_cast<iOSInstance*>(token);
    return instance->keep_alive();
}

extern "C" void process_let_die(void *token, void *preserver)
{
    iOSInstance *instance = reinterpret_cast<iOSInstance*>(token);
    return instance->let_die(reinterpret_cast<uv_async_t*>(preserver));
}
