/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
    void spawnedThread();
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
    static void DLOpen(const FunctionCallbackInfo<Value>& args);

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
#ifdef __APPLE__
    std::thread* node_main_thread = nullptr;
#endif
};

#endif //NODEDROID_NODEINSTANCE_H
