/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */

#include <jni.h>

#ifdef DEBUG
#include <sys/types.h>
#include <android/log.h>
#include <unistd.h>
#include <cstdio>
#include <pthread.h>

static int pfd[2];
static pthread_t thr;
static const char *tag = "myapp";

static void *thread_func(void*)
{
    ssize_t rdsz;
    char buf[128];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    return nullptr;
}

int start_logger(const char *app_name)
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, nullptr, _IOLBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&thr, nullptr, thread_func, nullptr) == -1)
        return -1;
    pthread_detach(thr);
    return 0;
}
#endif

static jobject s_ClassLoader;
static jmethodID s_FindClassMethod;

jint JNI_OnLoad(JavaVM* vm, void*)
{

#ifdef DEBUG
    start_logger("LiquidCore");
#endif

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    auto randomClass = env->FindClass("org/liquidplayer/javascript/JNIJSContextGroup");
    jclass classClass = env->GetObjectClass(randomClass);
    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
                                                 "()Ljava/lang/ClassLoader;");
    s_ClassLoader = env->NewGlobalRef(env->CallObjectMethod(randomClass, getClassLoaderMethod));
    s_FindClassMethod = env->GetMethodID(classLoaderClass, "findClass",
                                         "(Ljava/lang/String;)Ljava/lang/Class;");

    return JNI_VERSION_1_6;
}

jclass findClass(JNIEnv *env, const char* name)
{
    jstring clsname =  env->NewStringUTF(name);
    jclass clazz = static_cast<jclass>(env->CallObjectMethod(s_ClassLoader, s_FindClassMethod, clsname));
    env->DeleteLocalRef(clsname);
    return clazz;
}