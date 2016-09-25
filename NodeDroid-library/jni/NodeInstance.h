//
// Created by Eric on 9/9/16.
//

#ifndef NODEDROID_NODEINSTANCE_H
#define NODEDROID_NODEINSTANCE_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <jni.h>
#include <android/log.h>
#include <thread>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "node.h"
#include "uv.h"
#include "node_mutex.h"
#include "env.h"
#include "env-inl.h"
#if HAVE_OPENSSL
#include "node_crypto.h"
#endif

#include "JSJNI.h"

using namespace node;

class NodeInstance {
public:
    NodeInstance(JNIEnv* env, jobject thiz);
    virtual ~NodeInstance();

private:
    Environment* CreateEnvironment(Isolate* isolate,
                                   Local<Context> context,
                                   NodeInstanceData* instance_data);
    int StartNodeInstance(void* arg);
    int Start(int argc, char *argv[]);
    void spawnedThread();

    static void WaitForInspectorDisconnect(Environment* env);
    static bool DomainHasErrorHandler(const Environment* env,
                                  const Local<Object>& domain);
    static bool DomainsStackHasErrorHandler(const Environment* env);
    static bool ShouldAbortOnUncaughtException(Isolate* isolate);

    static void node_main_task(void *inst);

    static void Destruct();
    static bool StartInspector(Environment *env, int port, bool wait);
    static void PumpMessageLoop(Isolate* isolate);

    static void Chdir(const FunctionCallbackInfo<Value>& args);
    static void Cwd(const FunctionCallbackInfo<Value>& args);

private:
    Mutex node_isolate_mutex;
    v8::Isolate* node_isolate = nullptr;
    std::function<void(Persistent<Context,CopyablePersistentTraits<Context>>)> *onV8ContextCallback;

    bool debug_wait_connect = false;
    bool trace_sync_io = false;
    bool use_debug_agent = false;

    JavaVM *m_jvm = nullptr;
    jobject m_JavaThis = nullptr;

    std::thread* node_main_thread = nullptr;
};

#endif //NODEDROID_NODEINSTANCE_H
