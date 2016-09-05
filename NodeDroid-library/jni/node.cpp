//
// Created by Eric on 8/22/16.
//

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
#include "JSJNI.h"

static std::thread* node_main_thread;
static std::thread* stdout_thread;

static int pfd[2];
static volatile bool stopit = false;

static void node_stdout_thread()
{
    ssize_t rdsz;
    char buf[128];
    while(!stopit && (rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz - 1] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_DEBUG, "node", buf);
    }
}

static v8::Persistent<v8::Context,v8::CopyablePersistentTraits<v8::Context>> context;

static void node_store_context(v8::Persistent<v8::Context,v8::CopyablePersistentTraits<v8::Context>> ctx) {
    Isolate *isolate = Isolate::GetCurrent();
    V8_ISOLATE(isolate);

    context = ctx;
    Local<Context> localCtx = Local<Context>::New(isolate, context);
    Handle<Boolean> done = Boolean::New(isolate, false);
    localCtx->Global()->Set(String::NewFromUtf8(isolate, "nodedroid_done"), done );
    __android_log_write(ANDROID_LOG_DEBUG, "nodedroid", "Don't worry, we got here without fail");
}

static void node_main_task(std::string str)
{
    __android_log_write(ANDROID_LOG_DEBUG, "nodedroid", str.c_str());
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    stdout_thread = new std::thread(node_stdout_thread);
    stdout_thread->detach();
    enum { kMaxArgs = 64 };
    char cmd[] = "node -e wait=function(){if(!nodedroid_done)setTimeout(wait,1000);};wait();";

    int argc = 0;
    char *argv[kMaxArgs];

    char *p2 = strtok(cmd, " ");
    while (p2 && argc < kMaxArgs-1)
      {
        argv[argc++] = p2;
        p2 = strtok(0, " ");
      }
    argv[argc] = 0;

    std::function<void(v8::Persistent<v8::Context,v8::CopyablePersistentTraits<v8::Context>>)> f =
        node_store_context;

    __android_log_write(ANDROID_LOG_DEBUG, "nodedroid", "Starting Node");

    int ret = node::StartGetContext(argc, argv, &f);

    stopit = true;

    char buf[128];
    sprintf(buf, "Node exit : %d", ret);
    __android_log_write(ANDROID_LOG_DEBUG, "nodedroid", buf);
}

extern "C" JNIEXPORT void JNICALL Java_org_liquidplayer_nodedroid_Node_start(
    __attribute__((unused))JNIEnv* env,
    __attribute__((unused))jobject thiz)
{
    __android_log_write(ANDROID_LOG_DEBUG, "nodedroid", "JNI START");

    node_main_thread = new std::thread(node_main_task,"Node main thread");
}

extern "C" JNIEXPORT void JNICALL Java_org_liquidplayer_nodedroid_Node_exit(
    __attribute__((unused))JNIEnv* env,
    __attribute__((unused))jobject thiz)
{

    uv_work_t *req = new uv_work_t();
    req->data = NULL;

    int status = uv_queue_work(
        uv_default_loop(),
        req,
        [](uv_work_t* req)->void{

    },
        [](uv_work_t* req, int status)->void{

        __android_log_write(ANDROID_LOG_DEBUG, "nodedroid", "JNI EXIT");
        Isolate *isolate = Isolate::GetCurrent();
        V8_ISOLATE(isolate);

        Local<v8::Context> localCtx = Local<v8::Context>::New(isolate, context);
        Handle<v8::Boolean> done = v8::Boolean::New(isolate, true);
        localCtx->Global()->Set(v8::String::NewFromUtf8(isolate, "nodedroid_done"), done );

        context.Reset();

    });
}