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

#include "v8.h"
#include "libplatform/libplatform.h"
#include "uv.h"

using namespace v8;

#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_v8_##package##_##f
#define PARAMS JNIEnv* env, jobject thiz

class Retainer {
public:
    Retainer() {
        m_count = 1;
        m_debug_mutex.lock();
        bool found = (std::find(m_debug.begin(), m_debug.end(), this) != m_debug.end());
        m_debug.push_front(this);
        m_debug_mutex.unlock();
        if (found) {
            __android_log_write(ANDROID_LOG_DEBUG, "Retainer", "HOLY SHIT! IT'S ALREADY HERE!");
        }
    }

    virtual void retain() {
        m_count++;
    }

    virtual void release() {
        char buf[128];
        sprintf(buf, "count = %d", m_count-1);
        __android_log_write(ANDROID_LOG_DEBUG, "Retainer", buf);
        if (--m_count == 0)
            delete this;
    }

protected:
    virtual ~Retainer() {
        m_debug_mutex.lock();
        m_debug.remove(this);
        char buf[128];
        sprintf(buf, "hanging chads = %d", m_debug.size());
        m_debug_mutex.unlock();
        __android_log_write(ANDROID_LOG_DEBUG, "Retainer", buf);
    }

protected:
    int m_count;

public:
    static std::list<Retainer*> m_debug;
    static std::mutex m_debug_mutex;
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

class ContextGroup : public Retainer {
public:
    ContextGroup();
    virtual Isolate* isolate() {
        return m_isolate;
    }

protected:
    virtual ~ContextGroup();

private:
    static void init_v8();
    static void dispose_v8();

    static Platform *s_platform;
    static int s_init_count;
    static std::mutex s_mutex;

    Isolate *m_isolate;
    Isolate::CreateParams m_create_params;
    GenericAllocator m_allocator;
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

protected:
    virtual ~JSContext();

private:
    Persistent<Context, CopyablePersistentTraits<Context>> m_context;
    Persistent<Object, CopyablePersistentTraits<Object>> m_globalObject;
    ContextGroup *m_isolate;
};

#define V8_ISOLATE(iso) \
        Locker locker(iso); \
        Isolate::Scope isolate_scope_(iso); \
        HandleScope handle_scope_(iso)

#define V8_ISOLATE_CTX(ctx,iso,Ctx) \
        JSContext *context_ = reinterpret_cast<JSContext*>(ctx); \
        Isolate * iso = context_->isolate(); \
        V8_ISOLATE(iso); \
        Local<Context> Ctx = context_->Value(); \
        Context::Scope context_scope_(Ctx)

#endif