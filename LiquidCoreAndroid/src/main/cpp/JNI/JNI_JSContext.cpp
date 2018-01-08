//
// JSContext.cpp
//
// AndroidJSCore project
// https://github.com/ericwlange/AndroidJSCore/
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014 Eric Lange. All rights reserved.

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

#include "JNI/JNI.h"
#include "JSC/JSC.h"

static jobject s_ClassLoader;
static jmethodID s_FindClassMethod;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
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
    return static_cast<jclass>(env->CallObjectMethod(s_ClassLoader,
        s_FindClassMethod, env->NewStringUTF(name)));
}

NATIVE(JNIJSContextGroup,jobject,create) (PARAMS)
{
    // Maintain compatibility at the ContextGroup level with JSC by using JSContextGroupCreate()
    return SharedWrap<ContextGroup>::New(
        env,
        const_cast<OpaqueJSContextGroup*>(JSContextGroupCreate())->ContextGroup::shared_from_this()
    );
}

NATIVE(JNIJSContextGroup,jboolean,isManaged) (PARAMS)
{
    auto group = SharedWrap<ContextGroup>::Shared(env, thiz);
    return (jboolean) (group && group->Loop());
}

NATIVE(JNIJSContextGroup,void,runInContextGroup) (PARAMS, jobject thisObj, jobject runnable) {
    auto group = SharedWrap<ContextGroup>::Shared(env, thiz);

    if (group && group->Loop() && std::this_thread::get_id() != group->Thread()) {
        group->m_async_mutex.lock();

        struct Runnable *r = new struct Runnable;
        r->thiz = env->NewGlobalRef(thisObj);
        r->runnable = env->NewGlobalRef(runnable);
        r->c_runnable = nullptr;
        env->GetJavaVM(&r->jvm);
        group->m_runnables.push_back(r);

        if (!group->m_async_handle) {
            group->m_async_handle = new uv_async_t();
            group->m_async_handle->data = new ContextGroupData(group);
            uv_async_init(group->Loop(), group->m_async_handle, ContextGroup::callback);
            uv_async_send(group->m_async_handle);
        }
        group->m_async_mutex.unlock();
    } else {
        jclass cls = env->GetObjectClass(thisObj);
        jmethodID mid;
        do {
            mid = env->GetMethodID(cls,"inContextCallback","(Ljava/lang/Runnable;)V");
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == NULL || env->ExceptionCheck()) {
                if (super != NULL) env->DeleteLocalRef(super);
                __android_log_assert("FAIL", "runInContextGroup",
                    "Internal error.  Can't call back.");
                return;
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        env->CallVoidMethod(thisObj, mid, runnable);
    }
}

NATIVE(JNIJSContextGroup,void,Finalize) (PARAMS, long reference)
{
    SharedWrap<ContextGroup>::Dispose(reference);
}

NATIVE(JNIJSContext,jobject,createInGroup) (PARAMS,jobject grp)
{
    auto group = SharedWrap<ContextGroup>::Shared(env, grp);
    jobject ctx;
    { V8_ISOLATE(group,isolate)
        ctx = SharedWrap<JSContext>::New(env, JSContext::New(group, Context::New(isolate)));
    V8_UNLOCK() }

    return ctx;
}

NATIVE(JNIJSContext,jobject,create) (PARAMS)
{
    jobject jGroup = Java_org_liquidplayer_javascript_JNIJSContextGroup_create(env, thiz);
    return Java_org_liquidplayer_javascript_JNIJSContext_createInGroup(env, thiz, jGroup);
}

NATIVE(JNIJSContext,void,Finalize) (PARAMS, long reference)
{
    SharedWrap<JSContext>::Dispose(reference);
}

NATIVE(JNIJSContext,jobject,getGlobalObject) (PARAMS)
{
    jobject v=nullptr;
    auto ctx = SharedWrap<JSContext>::Shared(env, thiz);

    V8_ISOLATE_CTX(ctx,isolate,Ctx)
        v = SharedWrap<JSValue>::New(env, ctx->Global());
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSContext,jobject,getGroup) (PARAMS)
{
    auto context = SharedWrap<JSContext>::Shared(env, thiz);
    return SharedWrap<ContextGroup>::New(env, context->Group());
}

NATIVE(JNIJSContext,jobject,evaluateScript) (PARAMS, jstring script,
    jstring sourceURL, jint startingLineNumber)
{
    auto ctx = SharedWrap<JSContext>::Shared(env, thiz);

    const char *_script = env->GetStringUTFChars(script, NULL);
    const char *_sourceURL = env->GetStringUTFChars(sourceURL, NULL);

    JNIReturnObject ret(env);
    {
        V8_ISOLATE(ctx->Group(), isolate)
            TryCatch trycatch(isolate);
            Local<Context> context;

            context = ctx->Value();
            Context::Scope context_scope_(context);

            std::shared_ptr<JSValue> exception;

            ScriptOrigin script_origin(
                String::NewFromUtf8(isolate, _sourceURL, NewStringType::kNormal).ToLocalChecked(),
                Integer::New(isolate, startingLineNumber)
            );

            MaybeLocal<String> source = String::NewFromUtf8(isolate, _script, NewStringType::kNormal);
            MaybeLocal<Script> script;

            MaybeLocal<Value>  result;
            if (source.IsEmpty()) {
                exception = JSValue::New(ctx, trycatch.Exception());
            }

            if (!exception) {
                script = Script::Compile(context, source.ToLocalChecked(), &script_origin);
                if (script.IsEmpty()) {
                    exception = JSValue::New(ctx, trycatch.Exception());
                }
            }

            if (!exception) {
                result = script.ToLocalChecked()->Run(context);
                if (result.IsEmpty()) {
                    exception = JSValue::New(ctx, trycatch.Exception());
                }
            }

            if (!exception) {
                std::shared_ptr<JSValue> value =
                    JSValue::New(ctx, result.ToLocalChecked());
                ret.SetReference(SharedWrap<JSValue>::New(env, value));
            }

            if (exception) {
                ret.SetException(SharedWrap<JSValue>::New(env, exception));
            }
        V8_UNLOCK()
    }

    env->ReleaseStringUTFChars(script, _script);
    env->ReleaseStringUTFChars(sourceURL, _sourceURL);

    return ret.ToJava();
}
