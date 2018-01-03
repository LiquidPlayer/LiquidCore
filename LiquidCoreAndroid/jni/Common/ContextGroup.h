//
// ContextGroup.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 - 2018 Eric Lange. All rights reserved.

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
#ifndef LIQUIDCORE_CONTEXTGROUP_H
#define LIQUIDCORE_CONTEXTGROUP_H

#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <mutex>
#include <list>
#include <algorithm>
#include <set>
#include <thread>
#include <list>
#include <functional>
#include <map>
#include <memory>

#include "Common/ManagedObject.h"
#include "v8.h"
#include "libplatform/libplatform.h"
#include "uv.h"

using namespace v8;

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
    std::function<void()> c_runnable;
};

class ContextGroup : public std::enable_shared_from_this<ContextGroup> {
public:
    ContextGroup();
    ContextGroup(Isolate *isolate, uv_loop_t *uv_loop);

    virtual inline Isolate* isolate() { return m_isDefunct ? nullptr : m_isolate; }
    virtual inline uv_loop_t * Loop() { return m_isDefunct ? nullptr : m_uv_loop; }
    virtual inline bool IsDefunct() { return m_isDefunct; }
    virtual inline std::thread::id Thread() { return m_thread_id; }

    virtual void sync(std::function<void()> runnable);
    virtual void async(std::function<void()> runnable, bool queue_only);
    virtual void RegisterGCCallback(void (*cb)(GCType type, GCCallbackFlags flags, void*), void *);
    virtual void UnregisterGCCallback(void (*cb)(GCType type, GCCallbackFlags flags,void*), void *);
    virtual void ManageObject(std::shared_ptr<ManagedObject> obj);
    virtual void Dispose();

    static void init_v8();
    static inline std::mutex *Mutex() { return &s_mutex; }
    static inline v8::Platform * Platform() { return s_platform; }
    static void callback(uv_async_t* handle);
    static void StaticGCPrologueCallback(Isolate *isolate, GCType type, GCCallbackFlags flags);

protected:
    virtual ~ContextGroup();
    virtual void GCPrologueCallback(GCType type, GCCallbackFlags flags);

private:
    static void dispose_v8();

    static v8::Platform *s_platform;
    static int s_init_count;
    static std::mutex s_mutex;
    static std::map<Isolate *, ContextGroup *> s_isolate_map;

    Isolate *m_isolate;
    Isolate::CreateParams m_create_params;
    static GenericAllocator s_allocator;
    bool m_manage_isolate;
    uv_loop_t *m_uv_loop;
    std::thread::id m_thread_id;
    std::vector<std::weak_ptr<ManagedObject>> m_managedObjects;
    bool m_isDefunct;

    struct GCCallback {
        void (*cb)(GCType type, GCCallbackFlags flags, void*);
        void *data;
    };
    std::list<std::unique_ptr<struct GCCallback>> m_gc_callbacks;

public:
    uv_async_t *m_async_handle;
    std::list<struct Runnable *> m_runnables;
    std::mutex m_async_mutex;
    std::recursive_mutex m_scheduling_mutex;
};

class ContextGroupData {
public:
    ContextGroupData(std::shared_ptr<ContextGroup> cg) : m_context_group(cg) {}
    virtual ~ContextGroupData() { m_context_group.reset(); }
    std::shared_ptr<ContextGroup> m_context_group;
};

#endif //LIQUIDCORE_CONTEXTGROUP_H
