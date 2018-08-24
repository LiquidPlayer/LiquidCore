//
// NodeInstance.h
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
#ifndef NODEDROID_NODEINSTANCE_H
#define NODEDROID_NODEINSTANCE_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __ANDROID__
# include <jni.h>
# include <android/log.h>
#endif
#include <thread>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <map>

#include "node.h"
#include "uv.h"
#include "node_mutex.h"
#include "env.h"
#include "env-inl.h"
#if HAVE_OPENSSL
#include "node_crypto.h"
#endif
#include "node_debug_options.h"

#ifdef __ANDROID__
# include "Common/Common.h"
# include <JavaScriptCore/JavaScript.h>
#endif

using namespace node;
using namespace v8;

typedef void (*OnNodeStartedCallback)(void* data, JSContextRef ctx, JSContextGroupRef group);
typedef void (*OnNodeExitCallback)(void* data, int code);

class NodeInstance {
public:
#ifdef __ANDROID__
    NodeInstance(JNIEnv* env, jobject thiz);
#endif
    NodeInstance(OnNodeStartedCallback onStart, OnNodeExitCallback onExit, void* data);
    virtual ~NodeInstance();

private:
    void StartInspector(Environment* env, const char* path,
                        DebugOptions debug_options);

    inline int StartInstance(uv_loop_t* event_loop,
                 int argc, const char* const* argv,
                 int exec_argc, const char* const* exec_argv);
    inline int StartInstance(void* group, IsolateData* isolate_data,
                 int argc, const char* const* argv,
                 int exec_argc, const char* const* exec_argv);
    inline void PlatformInit();

    int StartInstance(int argc, char *argv[]);
    void spawnedThread();

    static void WaitForInspectorDisconnect(Environment* env);
    static bool DomainHasErrorHandler(const Environment* env,
                                  const Local<Object>& domain);
    static bool DomainsStackHasErrorHandler(const Environment* env);
    static bool ShouldAbortOnUncaughtException(Isolate* isolate);

    static void node_main_task(void *inst);

    static void Chdir(const FunctionCallbackInfo<Value>& args);
    static void Cwd(const FunctionCallbackInfo<Value>& args);
    static void Exit(const FunctionCallbackInfo<Value>& args);
    static void Abort(const FunctionCallbackInfo<Value>& args);
    static void Kill(const FunctionCallbackInfo<Value>& args);
    static void OnFatalError(const char* location, const char* message);

private:
    Mutex node_isolate_mutex;
    v8::Isolate* node_isolate = nullptr;

    bool trace_sync_io = false;
    bool v8_is_profiling = false;
    bool abort_on_uncaught_exception = false;
    bool force_async_hooks_checks = false;
    bool track_heap_objects = false;
    bool trace_enabled = false;
#ifdef __ANDROID__
    JavaVM *m_jvm = nullptr;
    jobject m_JavaThis = nullptr;
#endif
    uv_key_t thread_local_env;

    void NotifyStart(JSContextRef ctx, JSContextGroupRef group);

    OnNodeStartedCallback on_start = nullptr;
    OnNodeExitCallback on_exit = nullptr;
    void * callback_data = nullptr;
protected:
    uv_loop_t *m_loop;
    std::thread* node_main_thread = nullptr;
};

#endif //NODEDROID_NODEINSTANCE_H
