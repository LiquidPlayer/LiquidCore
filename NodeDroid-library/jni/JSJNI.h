//
// JSJNI.h
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2016 Eric Lange. All rights reserved.

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

#ifndef _JSJNI_H
#define _JSJNI_H

#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <mutex>
#include <list>
#include <algorithm>
#include <set>
#include <thread>
#include <list>

#include "v8.h"
#include "libplatform/libplatform.h"
#include "uv.h"

using namespace v8;

#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_v8_##package##_##f
#define PARAMS JNIEnv* env, jobject thiz

//#define DEBUG_RETAINER 1

class Retainer {
public:
    Retainer() {
        m_count = 1;
#ifdef DEBUG_RETAINER
        m_debug_mutex.lock();
        m_debug.push_front(this);
        m_debug_mutex.unlock();
#endif
    }

    virtual void retain() {
        m_count++;
    }

    virtual void release() {
        if (--m_count == 0)
            delete this;
    }

protected:
    virtual ~Retainer() {
#ifdef DEBUG_RETAINER
        m_debug_mutex.lock();
        m_debug.remove(this);
        m_debug_mutex.unlock();
#endif
    }

protected:
    int m_count;

public:
#ifdef DEBUG_RETAINER
    static std::list<Retainer*> m_debug;
    static std::mutex m_debug_mutex;
#endif
};

class GenericAllocator : public ArrayBuffer::Allocator {
public:
    GenericAllocator() {}
    virtual ~GenericAllocator() {}
    virtual void* Allocate(size_t length) {
        unsigned char * mem =  (unsigned char *) malloc(length);
        memset(mem, 0, length);
        return (void*)mem;
    }
    virtual void* AllocateUninitialized(size_t length) {
        return malloc(length);
    }
    virtual void Free(void* data, size_t length) {
        free(data);
    }
};

struct Runnable {
    jobject thiz;
    jobject runnable;
    JavaVM *jvm;
};

class ContextGroup : public Retainer {
public:
    ContextGroup();
    ContextGroup(Isolate *isolate, uv_loop_t *uv_loop);
    virtual Isolate* isolate() {
        return m_isolate;
    }
    virtual uv_loop_t * Loop() {
        return m_uv_loop;
    }
    virtual std::thread::id Thread() {
        return m_thread_id;
    }
    virtual std::recursive_mutex& Locker() {
        return m_isolate_mutex;
    }

    static void init_v8();
    static std::mutex *Mutex() { return &s_mutex; }
    static v8::Platform * Platform() { return s_platform; }
    static void callback(uv_async_t* handle);

protected:
    virtual ~ContextGroup();

private:
    static void dispose_v8();

    static v8::Platform *s_platform;
    static int s_init_count;
    static std::mutex s_mutex;

    Isolate *m_isolate;
    Isolate::CreateParams m_create_params;
    GenericAllocator m_allocator;
    bool m_manage_isolate;
    uv_loop_t *m_uv_loop;
    std::thread::id m_thread_id;
    std::recursive_mutex m_isolate_mutex;

public:
    uv_async_t *m_async_handle;
    std::list<struct Runnable *> m_runnables;
    std::mutex m_async_mutex;
};

class JSContext;

template <typename T>
class JSValue : public Retainer {
public:
    virtual Local<T> Value();
    virtual Isolate* isolate();
    virtual ContextGroup* Group();
    static JSValue<T> *New(JSContext* context, Local<T> val);

protected:
    JSValue(JSContext* context, Local<T> val);
    JSValue() {}
    virtual ~JSValue();

protected:
    Persistent<T, CopyablePersistentTraits<T>> m_value;
    JSContext *m_context;
    bool m_isUndefined;
    bool m_isNull;

friend class JSContext;
};

// CRTP sucks
class JSContext : public Retainer {
public:
    JSContext(ContextGroup* isolate, Local<Context> val);
    virtual JSValue<Object> * Global();
    virtual Local<Context>    Value();
    virtual Isolate *         isolate();
    virtual ContextGroup *    Group();
    virtual void SetDefunct();
    virtual bool IsDefunct() { return m_isDefunct; }

    virtual void retain(JSValue<v8::Value>* value);
    virtual void release(JSValue<v8::Value>* value);
    virtual void retain(JSValue<v8::Object>* value);
    virtual void release(JSValue<v8::Object>* value);
    virtual void retain() { Retainer::retain(); }
    virtual void release() { Retainer::release(); }

protected:
    virtual ~JSContext();

private:
    Persistent<Context, CopyablePersistentTraits<Context>> m_context;
    Persistent<Object, CopyablePersistentTraits<Object>> m_globalObject;
    ContextGroup *m_isolate;
    bool m_isDefunct;
    std::set<JSValue<v8::Value>*> m_value_set;
    std::set<JSValue<v8::Object>*> m_object_set;
};

#define V8_ISOLATE(group,iso) \
        Isolate *iso = group->isolate(); \
        ContextGroup* group_ = group; \
        if (!group->Loop()) group->Locker().lock(); \
        Isolate::Scope isolate_scope_(iso); \
        HandleScope handle_scope_(iso)

#define V8_ISOLATE_CTX(ctx,iso,Ctx) \
        JSContext *context_ = reinterpret_cast<JSContext*>(ctx); \
        V8_ISOLATE(context_->Group(),iso); \
        Local<Context> Ctx = context_->Value(); \
        Context::Scope context_scope_(Ctx)

#define V8_UNLOCK() \
        if (!group_->Loop()) group_->Locker().unlock()


#endif