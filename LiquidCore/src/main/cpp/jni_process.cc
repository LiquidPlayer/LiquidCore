/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "node.h"
#include <jni.h>
#include <string.h>
#include "JNI/JNI.h"
#include "JSC/JSC.h"
#include "node_instance.h"

#undef NATIVE
#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_node_##package##_##f
#undef PARAMS
#define PARAMS JNIEnv* env, jobject thiz

struct AndroidInstance : NodeInstance {

    AndroidInstance(JNIEnv *env, jobject thiz) {
        if (!s_jvm)
            env->GetJavaVM(&s_jvm);
        m_JavaThis = env->NewGlobalRef(thiz);
    }

    ~AndroidInstance() {
        JNIEnv *env;
        int getEnvStat = s_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            s_jvm->AttachCurrentThread(&env, nullptr);
        }

        env->DeleteGlobalRef(m_JavaThis);

        if (getEnvStat == JNI_EDETACHED) {
            s_jvm->DetachCurrentThread();
        }
    }

    void spawnedThread();
    void NotifyStart(JSContextRef ctxRef, JSContextGroupRef groupRef);

    jobject m_JavaThis;
    static JavaVM *s_jvm;
};
static std::mutex s_instance_mutex;
static std::map<std::thread::id, AndroidInstance*> s_instances;

void AndroidInstance::spawnedThread()
{
    {
        std::unique_lock<std::mutex> lock(s_instance_mutex);
        s_instances[std::this_thread::get_id()] = this;
    }

    enum { kMaxArgs = 64 };
    char cmd[60];

    setenv("NODE_PATH", "/home/node_modules", true);
    strcpy(cmd, "node -e global.__nodedroid_onLoad();");

    int argc = 0;
    char *argv[kMaxArgs];

    char *p2 = strtok(cmd, " ");
    while (p2 && argc < kMaxArgs-1)
    {
        argv[argc++] = p2;
        p2 = strtok(0, " ");
    }
    argv[argc] = 0;

    int ret = node::Start(argc, argv);

    {
        std::unique_lock<std::mutex> lock(s_instance_mutex);
        s_instances.erase(std::this_thread::get_id());
    }

    JNIEnv *env;
    int getEnvStat = s_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        s_jvm->AttachCurrentThread(&env, nullptr);
    }

    jclass cls = env->GetObjectClass(m_JavaThis);
    jmethodID mid;
    do {
        mid = env->GetMethodID(cls,"onNodeExit","(J)V");
        if (!env->ExceptionCheck()) break;
        env->ExceptionClear();
        jclass super = env->GetSuperclass(cls);
        env->DeleteLocalRef(cls);
        if (super == NULL || env->ExceptionCheck()) {
            if (super != NULL) env->DeleteLocalRef(super);
            if (getEnvStat == JNI_EDETACHED) {
                s_jvm->DetachCurrentThread();
            }
            return;
        }
        cls = super;
    } while (true);
    env->DeleteLocalRef(cls);

    env->CallVoidMethod(m_JavaThis, mid, (jlong)ret);

    env->DeleteGlobalRef(m_JavaThis);

    if (getEnvStat == JNI_EDETACHED) {
        s_jvm->DetachCurrentThread();
    }
}

void AndroidInstance::NotifyStart(JSContextRef ctxRef, JSContextGroupRef groupRef)
{
    JNIEnv *jenv;
    int getEnvStat = s_jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        s_jvm->AttachCurrentThread(&jenv, nullptr);
    }

    jclass cls = jenv->GetObjectClass(m_JavaThis);
    jmethodID mid;
    do {
        mid = jenv->GetMethodID(cls,"onNodeStarted","(JJJ)V");
        if (!jenv->ExceptionCheck()) break;
        jenv->ExceptionClear();
        jclass super = jenv->GetSuperclass(cls);
        jenv->DeleteLocalRef(cls);
        if (super == nullptr || jenv->ExceptionCheck()) {
            if (super != nullptr) jenv->DeleteLocalRef(super);
            if (getEnvStat == JNI_EDETACHED) {
                s_jvm->DetachCurrentThread();
            }
            __android_log_assert("FAIL","NotifyStart","This is bad");
        }
        cls = super;
    } while (true);
    jenv->DeleteLocalRef(cls);

    auto group = const_cast<OpaqueJSContextGroup*>(groupRef);

    jenv->CallVoidMethod(m_JavaThis, mid,
                         SharedWrap<JSContext>::New(ctxRef->Context()),
                         SharedWrap<ContextGroup>::New(group->ContextGroup::shared_from_this()),
                         reinterpret_cast<jlong>(ctxRef)
    );

    if (getEnvStat == JNI_EDETACHED) {
        s_jvm->DetachCurrentThread();
    }
}

v8::Local<v8::Context> NodeInstance::NewContext(v8::Isolate *isolate, JSContextGroupRef groupRef,
                                  JSGlobalContextRef *ctxRef)
{
    v8::EscapableHandleScope scope(isolate);
    JSGlobalContextRef dummy;
    if (!ctxRef) ctxRef = &dummy;

    JSClassRef globalClass = nullptr;
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.attributes |= kJSClassAttributeNoAutomaticPrototype;
        globalClass = JSClassCreate(&definition);
        *ctxRef = JSGlobalContextCreateInGroup(groupRef, globalClass);
    }
    JSClassRelease(globalClass);
    auto java_node_context = (*ctxRef)->Context();
    v8::Local<v8::Context> context = java_node_context->Value();
    return scope.Escape(context);
}

JSContextGroupRef NodeInstance::GroupFromIsolate(v8::Isolate *isolate, uv_loop_t* event_loop)
{
    return &* OpaqueJSContextGroup::New(isolate, event_loop);
}

void NodeInstance::NotifyStart(JSContextRef ctxRef, JSContextGroupRef groupRef)
{
    AndroidInstance * thiz;
    {
        std::unique_lock<std::mutex> lock(s_instance_mutex);
        thiz = s_instances[std::this_thread::get_id()];
    }
    thiz->NotifyStart(ctxRef, groupRef);
}

JavaVM * AndroidInstance::s_jvm(nullptr);

NATIVE(Process,jlong,start) (PARAMS)
{
    auto instance = new AndroidInstance(env, thiz);
    return reinterpret_cast<jlong>(instance);
}

NATIVE(Process,void,runInThread) (PARAMS, jlong ref)
{
    auto instance = reinterpret_cast<AndroidInstance*>(ref);
    instance->spawnedThread();
}

NATIVE(Process,void,dispose) (PARAMS, jlong ref)
{
    delete reinterpret_cast<NodeInstance*>(ref);
}

NATIVE(Process,void,setFileSystem) (PARAMS, jlong contextRef, jlong fsObjectRef)
{
    auto ctx = SharedWrap<JSContext>::Shared(contextRef);
    if (!ISPOINTER(fsObjectRef)) {
        __android_log_assert("!ISPOINTER", "setFileSystem", "SharedWrap<JSValue> is not a pointer");
    }
    auto fs = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(),fsObjectRef);
    V8_ISOLATE_CTX(ctx,isolate,context)

        Local<Object> globalObj = context->Global();
        Local<Object> fsObj = fs->Value()->ToObject(context).ToLocalChecked();

        Local<Private> privateKey = v8::Private::ForApi(isolate,
                                                        String::NewFromUtf8(isolate, "__fs"));
        globalObj->SetPrivate(context, privateKey, fsObj).FromJust();

    V8_UNLOCK();
}

NATIVE(Process,void,exposeRawFS) (PARAMS, jlong contextRef, jstring dir, jint mediaAccessMask)
{
    auto ctx = SharedWrap<JSContext>::Shared(contextRef);
    const char *c_string = env->GetStringUTFChars(dir, nullptr);
    V8_ISOLATE_CTX(ctx,isolate,context)
        auto globalObj = context->Global();
        auto privateKey = v8::Private::ForApi(isolate, String::NewFromUtf8(isolate, "__fs"));
        auto fs = globalObj->GetPrivate(context, privateKey).ToLocalChecked();
        auto access_ = fs.As<Object>()->Get(context,
                String::NewFromUtf8(isolate, "access_")).ToLocalChecked();
        access_.As<Object>()->Set(String::NewFromUtf8(isolate,c_string),
                Int32::New(isolate,mediaAccessMask));
    V8_UNLOCK();
    env->ReleaseStringUTFChars(dir, c_string);
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    ContextGroup::init_icu();
    node::InitializeNode();
    return JNI_VERSION_1_6;
}

extern "C" jlong JNICALL Java_org_liquidplayer_javascript_JSContext_getPlatform (JNIEnv* env, jclass klass)
{
    return reinterpret_cast<jlong>(node::GetPlatform());
}