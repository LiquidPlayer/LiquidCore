//
// nodedroid_file.cc
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
#include <jni.h>
#include <android/log.h>
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

#include "common.h"

using namespace node;

class NodeInstance {
public:
    NodeInstance(JNIEnv* env, jobject thiz);
    NodeInstance();
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
    static void Exit(const FunctionCallbackInfo<Value>& args);
    static void OnFatalError(const char* location, const char* message);

    static std::map<Environment*,NodeInstance*> instance_map;

private:
    Mutex node_isolate_mutex;
    v8::Isolate* node_isolate = nullptr;
    std::function<void(Persistent<Context,CopyablePersistentTraits<Context>>)> *onV8ContextCallback;

    bool debug_wait_connect = false;
    bool trace_sync_io = false;
    bool use_debug_agent = false;
    bool didExit = false;
    int  exit_code = 0;

    JavaVM *m_jvm = nullptr;
    jobject m_JavaThis = nullptr;

    std::thread* node_main_thread = nullptr;
};

#endif //NODEDROID_NODEINSTANCE_H
