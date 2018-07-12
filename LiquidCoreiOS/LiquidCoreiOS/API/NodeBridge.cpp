//
//  NodeBridge.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/7/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include <stdio.h>
#include <JavaScriptCore/JavaScript.h>
#include "NodeInstance.h"
#include "NodeBridge.h"
#include "v8.h"
#include "libplatform/libplatform.h"
#include "V82JSC.h"

class iOSInstance : NodeInstance {
public:
    iOSInstance(OnNodeStartedCallback onStart, OnNodeExitCallback onExit, void* data) :
        NodeInstance(onStart, onExit, data)
    {
        m_async_handle = nullptr;
    }

    void sync(ProcessThreadCallback callback, void *data)
    {
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

private:
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
};

static v8::Platform *s_platform = nullptr;
static int init_count = 0;

static void process_static_init()
{
    if (!init_count ++) {
        s_platform = v8::platform::CreateDefaultPlatform(4);
        v8::V8::InitializePlatform(s_platform);
        //v8::V8::Initialize();
    }
}

static void process_static_dispose()
{
    if (--init_count == 0) {
        v8::V8::Dispose();
        V8::ShutdownPlatform();
        delete s_platform;
        s_platform = nullptr;
    }
}

extern "C" void * process_start(OnNodeStartedCallback onStart, OnNodeExitCallback onExit, void* data)
{
    process_static_init();
    iOSInstance *instance = new iOSInstance(onStart, onExit, data);
    return reinterpret_cast<void*>(instance);
}

extern "C" void process_dispose(void *token)
{
    delete reinterpret_cast<iOSInstance*>(token);
    process_static_dispose();
}

extern "C" void process_set_filesystem(JSContextRef ctx, JSObjectRef fs)
{
    Isolate *isolate = V82JSC::ToIsolate(IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)]);
    HandleScope scope(isolate);
    
    v8::Local<v8::Context> context = LocalContextImpl::New(isolate, ctx);
    Context::Scope context_scope(context);
    
    Local<Object> globalObj = context->Global();
    Local<Object> fsObj = ValueImpl::New(V82JSC::ToContextImpl(context), fs).As<Object>();
    
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
