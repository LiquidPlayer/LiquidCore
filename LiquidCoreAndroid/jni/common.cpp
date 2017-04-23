//
// common.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
#include "common.h"
#include "node/NodeInstance.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>
#include <exception>
#include <condition_variable>

/**
 * class Retainer
 **/

#ifdef DEBUG_RETAINER
std::list<Retainer*> Retainer::m_debug;
std::mutex Retainer::m_debug_mutex;
#endif

/**
 * class JSContext
 **/

JSContext::JSContext(ContextGroup* isolate, Local<Context> val) {
    m_isolate = isolate;
    m_isolate->retain();
    m_context = Persistent<Context,CopyablePersistentTraits<Context>>(isolate->isolate(), val);
    m_isDefunct = false;
}

JSContext::~JSContext() {
    SetDefunct();
    m_isolate->release();
};

void JSContext::SetDefunct() {
    if (!m_isDefunct) {
        m_isDefunct = true;

        m_set_mutex.lock();
        while (m_value_set.size() > 0) {
            std::set<JSValue<v8::Value>*>::iterator it = m_value_set.begin();
            (*it)->release();
        }

        while (m_object_set.size() > 0) {
            std::set<JSValue<v8::Object>*>::iterator it = m_object_set.begin();
            (*it)->release();
        }

        while (m_array_set.size() > 0) {
            std::set<JSValue<v8::Array>*>::iterator it = m_array_set.begin();
            (*it)->release();
        }
        m_set_mutex.unlock();

        m_context.Reset();
    }
}

void JSContext::retain(JSValue<v8::Value>* value) {
    m_set_mutex.lock();
    m_value_set.insert(value);
    m_set_mutex.unlock();
}

void JSContext::release(JSValue<v8::Value>* value) {
    m_set_mutex.lock();
    m_value_set.erase(value);
    m_set_mutex.unlock();
}

void JSContext::retain(JSValue<v8::Object>* value) {
    m_set_mutex.lock();
    m_object_set.insert(value);
    m_set_mutex.unlock();
}

void JSContext::release(JSValue<v8::Object>* value) {
    m_set_mutex.lock();
    m_object_set.erase(value);
    m_set_mutex.unlock();
}

void JSContext::retain(JSValue<v8::Array>* value) {
    m_set_mutex.lock();
    m_array_set.insert(value);
    m_set_mutex.unlock();
}

void JSContext::release(JSValue<v8::Array>* value) {
    m_set_mutex.lock();
    m_array_set.erase(value);
    m_set_mutex.unlock();
}

JSValue<Object>* JSContext::Global() {
    return JSValue<Object>::New(this, Value()->Global());
}

Local<Context> JSContext::Value() {
    return Local<Context>::New(isolate(), m_context);
}

Isolate* JSContext::isolate() {
    return m_isolate->isolate();
}

ContextGroup* JSContext::Group() {
    return m_isolate;
}

/**
 * class ContextGroup
 **/

Platform *ContextGroup::s_platform = NULL;
int ContextGroup::s_init_count = 0;
std::mutex ContextGroup::s_mutex;
std::map<Isolate *, ContextGroup *> ContextGroup::s_isolate_map;

void ContextGroup::init_v8() {
    s_mutex.lock();
    if (s_init_count++ == 0) {
        // see: https://github.com/nodejs/node/issues/7918
        const char *flags = "--harmony-instanceof"; // " --expose_gc";
        V8::SetFlagsFromString(flags, strlen(flags));

        s_platform = platform::CreateDefaultPlatform(4);
        V8::InitializePlatform(s_platform);
        V8::Initialize();
    }

    s_mutex.unlock();
}

void ContextGroup::dispose_v8() {
    s_mutex.lock();
    // FIXME: Once disposed, an attempt to re-init will crash
    // For now, init once and never dispose
    //--s_init_count;
    if (s_init_count == 0) {
        V8::Dispose();
        V8::ShutdownPlatform();
        delete s_platform;
        s_platform = nullptr;
    }
    s_mutex.unlock();
}

GenericAllocator ContextGroup::s_allocator;

ContextGroup::ContextGroup() {
    init_v8();
    m_create_params.array_buffer_allocator = &s_allocator;
    m_isolate = Isolate::New(m_create_params);
    m_manage_isolate = true;
    m_uv_loop = nullptr;
    m_thread_id = std::this_thread::get_id();
    m_async_handle = nullptr;

    s_isolate_map[m_isolate] = this;
    m_gc_callbacks.clear();
    //m_isolate->AddGCPrologueCallback(StaticGCPrologueCallback);

    V8_ISOLATE(this,isolate)
        m_default_context = Persistent<Context,CopyablePersistentTraits<Context>>(isolate,
            Context::New(isolate));
    V8_UNLOCK()
}

ContextGroup::ContextGroup(Isolate *isolate, uv_loop_t *uv_loop) {
    m_isolate = isolate;
    m_manage_isolate = false;
    m_uv_loop = uv_loop;
    m_thread_id = std::this_thread::get_id();
    m_async_handle = nullptr;

    s_isolate_map[m_isolate] = this;
    m_gc_callbacks.clear();
    //m_isolate->AddGCPrologueCallback(StaticGCPrologueCallback);

    {
        Isolate::Scope isolate_scope_(m_isolate);
        HandleScope handle_scope_(m_isolate);

        m_default_context = Persistent<Context,CopyablePersistentTraits<Context>>(isolate,
            Context::New(isolate));
    }
}

void ContextGroup::SetDefaultContext(Local<Context> context)
{
    m_default_context.Reset();
    m_default_context = Persistent<Context,CopyablePersistentTraits<Context>>(context->GetIsolate(),
        context);
}

void ContextGroup::callback(uv_async_t* handle) {
    ContextGroup *group = reinterpret_cast<ContextGroup*>(handle->data);

    group->m_async_mutex.lock();
    struct Runnable *r = group->m_runnables.empty() ? nullptr : group->m_runnables.front();

    while (r) {
        group->m_async_mutex.unlock();

        if (r->c_runnable) {
            r->c_runnable();
        } else {
            JNIEnv *env;
            int getEnvStat = r->jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (getEnvStat == JNI_EDETACHED) {
                r->jvm->AttachCurrentThread(&env, NULL);
            }

            jclass cls = env->GetObjectClass(r->thiz);
            jmethodID mid;
            do {
                mid = env->GetMethodID(cls,"inContextCallback","(Ljava/lang/Runnable;)V");
                if (!env->ExceptionCheck()) break;
                env->ExceptionClear();
                jclass super = env->GetSuperclass(cls);
                env->DeleteLocalRef(cls);
                if (super == NULL || env->ExceptionCheck()) {
                    if (super != NULL) env->DeleteLocalRef(super);
                    if (getEnvStat == JNI_EDETACHED) {
                        r->jvm->DetachCurrentThread();
                    }
                    __android_log_assert("FAIL", "ContextGroup::callback",
                        "Can't find the class to call back?");
                    break;
                }
                cls = super;
            } while (true);
            env->DeleteLocalRef(cls);

            env->CallVoidMethod(r->thiz, mid, r->runnable);

            env->DeleteGlobalRef(r->thiz);
            env->DeleteGlobalRef(r->runnable);

            if (getEnvStat == JNI_EDETACHED) {
                r->jvm->DetachCurrentThread();
            }
        }

        group->m_async_mutex.lock();

        group->m_runnables.erase(group->m_runnables.begin());
        delete r;

        r = group->m_runnables.empty() ? nullptr : group->m_runnables.front();
    }
    // Close the handle.  We will create a new one if we
    // need another.  This keeps the node process from staying alive
    // indefinitely
    uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
        delete (uv_async_t*)h;
    });
    group->m_async_handle = nullptr;
    group->m_async_mutex.unlock();
}

void ContextGroup::RegisterGCCallback(void (*cb)(GCType, GCCallbackFlags, void*), void *data) {
    struct GCCallback *gc = new struct GCCallback;
    gc->cb = cb;
    gc->data = data;
    m_gc_callbacks.push_back(gc);
}

void ContextGroup::UnregisterGCCallback(void (*cb)(GCType, GCCallbackFlags, void*), void *data) {
    for (
        std::list<struct GCCallback*>::iterator it=m_gc_callbacks.begin();
        it!=m_gc_callbacks.end();
        ++it
    ) {

        if ((*it)->cb == cb && (*it)->data == data) {
            delete (*it);
            //m_gc_callbacks.erase(it);
        }
    }
}

void ContextGroup::GCPrologueCallback(GCType type, GCCallbackFlags flags) {
    for (
        std::list<struct GCCallback*>::iterator it=m_gc_callbacks.begin();
        it!=m_gc_callbacks.end();
        ++it
    ) {
        (*it)->cb(type, flags, (*it)->data);
    }
}

void ContextGroup::Clean() {
    if (!m_manage_isolate) {
        Isolate::Scope isolate_scope_(m_isolate);
        HandleScope handle_scope_(m_isolate);
        m_default_context.Reset();
    }
}

ContextGroup::~ContextGroup() {
    //Not really necessary at this point
    //m_isolate->RemoveGCPrologueCallback(StaticGCPrologueCallback);

    s_isolate_map.erase(m_isolate);
    if (m_manage_isolate) {
        auto dispose = [](Isolate *isolate) {
            // This is a hack to deal with the following failure message from V8
            // when executed during the Java finalizer (sometimes):
            // #
            // # Fatal error in v8::Isolate::Dispose()
            // # Disposing the isolate that is entered by a thread.
            // #
            // The only way to make this not occur is to (1) make sure we have entered
            // an isolate (in this case, we are creating a temp one) and (2)
            // execute outside of the finalizer thread.  I get why (1) is necessary
            // (see code for v8::Isolate::TearDown() in deps/v8/src/isolate.cc), but
            // I am confused as to why (2) is required.
            Isolate::CreateParams params;
            params.array_buffer_allocator = &s_allocator;
            Isolate *temp_isolate = Isolate::New(params);
            {
                temp_isolate->Enter();
                isolate->Dispose();
                temp_isolate->Exit();
            }
            temp_isolate->Dispose();
            dispose_v8();
        };
        std::thread(dispose, m_isolate).detach();
    } else {
        dispose_v8();
    }
}

void ContextGroup::sync(std::function<void()> runnable) {
    if (Loop() && std::this_thread::get_id() != Thread()) {

        std::condition_variable cv;
        bool signaled = false;

        struct Runnable *r = new struct Runnable;
        r->thiz = nullptr;
        r->runnable = nullptr;
        r->jvm = nullptr;
        r->c_runnable = [&]() {
            runnable();
            {
                std::lock_guard<std::mutex> lk(m_async_mutex);
                signaled = true;
            }
            cv.notify_one();
        };

        std::unique_lock<std::mutex> lk(m_async_mutex);
        m_runnables.push_back(r);

        if (!m_async_handle) {
            m_async_handle = new uv_async_t();
            m_async_handle->data = this;
            uv_async_init(Loop(), m_async_handle, ContextGroup::callback);
            uv_async_send(m_async_handle);
        }

        cv.wait(lk, [&]{return signaled;});
        lk.unlock();
    } else {
        runnable();
    }
}
