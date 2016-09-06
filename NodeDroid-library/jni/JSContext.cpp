//
// JSContext.cpp
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
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

#include "JSJNI.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>
#include <exception>

std::list<Retainer*> Retainer::m_debug;
std::mutex Retainer::m_debug_mutex;

template <typename T>
JSValue<T> * JSValue<T>::New(JSContext* context, Local<T> val) {
    if (val->IsObject()) {
        Local<Object> obj = val->ToObject(context->Value()).ToLocalChecked();
        Local<v8::Value> identifier =
            obj->GetHiddenValue(String::NewFromUtf8(context->isolate(), "__JSValue_ptr"));
        if (!identifier.IsEmpty() && identifier->IsNumber()) {
            // This object is already wrapped, let's re-use it
            JSValue<T> *value =
                reinterpret_cast<JSValue<T>*>(
                    (long)identifier->ToNumber(context->Value()).ToLocalChecked()->Value());
            value->retain();
            return value;
        } else {
            // First time wrap.  Create it new and mark it
            JSValue<T> *value = new JSValue<T>(context,val);
            obj->SetHiddenValue(String::NewFromUtf8(context->isolate(), "__JSValue_ptr"),
                Number::New(context->isolate(),(double)reinterpret_cast<long>(value)));
            return value;
        }
    } else {
        return new JSValue<T>(context,val);
    }
}

template <typename T>
JSValue<T>::JSValue(JSContext* context, Local<T> val) {
    if (val->IsUndefined()) {
        m_isUndefined = true;
        m_isNull = false;
    } else if (val->IsNull()) {
        m_isUndefined = false;
        m_isNull = true;
    } else {
        m_value = Persistent<T,CopyablePersistentTraits<T>>(context->isolate(), val);
        m_isUndefined = false;
        m_isNull = false;
    }
    m_context = context;
    m_context->retain();
}
template <typename T>
JSValue<T>::~JSValue<T>() {
    V8_ISOLATE(isolate());

    if (!m_isUndefined && !m_isNull) {
        if (Value()->IsObject()) {
            Local<Object> obj = Value()->ToObject(m_context->Value()).ToLocalChecked();
            // Clear wrapper pointer if it exists, in case this object is still held by JS
            obj->SetHiddenValue(String::NewFromUtf8(isolate(), "__JSValue_ptr"),
                Local<v8::Value>::New(isolate(),Undefined(isolate())));
        }
        m_value.Reset();
    }

    m_context->release();
}
template <typename T>
Local<T> JSValue<T>::Value() {
    if (m_isUndefined) {
        Local<v8::Value> undefined =
            Local<v8::Value>::New(isolate(),Undefined(isolate()));
        return *reinterpret_cast<Local<T> *>(&undefined);
    } else if (m_isNull) {
        Local<v8::Value> null =
            Local<v8::Value>::New(isolate(),Null(isolate()));
        return *reinterpret_cast<Local<T> *>(&null);
    } else {
        return Local<T>::New(isolate(), m_value);
    }
}

template <typename T>
Isolate* JSValue<T>::isolate() {
    return m_context->isolate();
}

template <typename T>
ContextGroup* JSValue<T>::Group() {
    return m_context->Group();
}

JSContext::JSContext(ContextGroup* isolate, Local<Context> val) {
    m_isolate = isolate;
    m_isolate->retain();
    m_context = Persistent<Context,CopyablePersistentTraits<Context>>(isolate->isolate(), val);
}
JSContext::~JSContext() {
    m_context.Reset();
    m_isolate->release();
};

JSValue<Object>* JSContext::Global() {
    return JSValue<Object>::New(this, Value()->Global());
}

Local<Context> JSContext::Value() {
    return Local<Context>::New(isolate(), m_context);
}

Isolate* JSContext::isolate() {
    return m_isolate->isolate();
}

ContextGroup* JSContext::Group() {
    return m_isolate;
}

Platform *ContextGroup::s_platform = NULL;
int ContextGroup::s_init_count = 0;
std::mutex ContextGroup::s_mutex;

void ContextGroup::init_v8() {
    s_mutex.lock();
    if (s_init_count == 0) {
        s_platform = platform::CreateDefaultPlatform();
        V8::InitializePlatform(s_platform);
        V8::Initialize();
        s_init_count ++;
    }
    s_mutex.unlock();
}

void ContextGroup::dispose_v8() {
/*
    s_mutex.lock();
    if (--s_init_count == 0) {
        V8::Dispose();
        V8::ShutdownPlatform();
        delete s_platform;
        s_platform = NULL;
    }
    s_mutex.unlock();
*/
}

ContextGroup::ContextGroup() {
    init_v8();
    m_create_params.array_buffer_allocator = &m_allocator;
    m_isolate = Isolate::New(m_create_params);
}
ContextGroup::~ContextGroup() {
    m_isolate->Dispose();

    dispose_v8();
}

struct Runnable {
    jobject thiz;
    jobject runnable;
};

NATIVE(JSContext,void,runInContext) (PARAMS, jobject runnable) {
/*
    uv_work_t *req = new uv_work_t();
    struct Runnable *r = new struct Runnable;
    r->thiz = thiz;
    r->runnable = runnable;
    req->data = (void*) r;

    int status = uv_queue_work(
        uv_main_loop(),
        req,
        [](uv_work_t* req)->void{
    },
        [](uv_work_t* req, int status)->void{

        struct Runnable r = (struct runnable*) req->data;

        JNIEnv *env;
        int getEnvStat = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            jvm->AttachCurrentThread(&env, NULL);
        }

        jclass cls = env->GetObjectClass(r->thiz);
        jmethodID mid;
        do {
            mid = env->GetMethodID(cls,"inContextCallback","(Ljava/lang/Runnable;)V");
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == NULL || env->ExceptionCheck()) {
                if (super != NULL) env->DeleteLocalRef(super);
                if (getEnvStat == JNI_EDETACHED) {
                    jvm->DetachCurrentThread();
                }
                delete r;
                return;
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        env->CallVoidMethod(r->thiz, mid, r->runnable);

        if (getEnvStat == JNI_EDETACHED) {
            jvm->DetachCurrentThread();
        }
        delete r;
    });
*/
    jclass cls = env->GetObjectClass(thiz);
    jmethodID mid;
    do {
        mid = env->GetMethodID(cls,"inContextCallback","(Ljava/lang/Runnable;)V");
        if (!env->ExceptionCheck()) break;
        env->ExceptionClear();
        jclass super = env->GetSuperclass(cls);
        env->DeleteLocalRef(cls);
        if (super == NULL || env->ExceptionCheck()) {
            if (super != NULL) env->DeleteLocalRef(super);
            return;
        }
        cls = super;
    } while (true);
    env->DeleteLocalRef(cls);

    env->CallVoidMethod(thiz, mid, runnable);
}

NATIVE(JSContextGroup,jlong,create) (PARAMS) {
    ContextGroup *group = new ContextGroup();
    return reinterpret_cast<long>(group);
}

NATIVE(JSContextGroup,jlong,retain) (PARAMS,jlong group) {
    ContextGroup *isolate = (ContextGroup*) group;
    isolate->retain();
    return group;
}

NATIVE(JSContextGroup,void,release) (PARAMS,jlong group) {
    ContextGroup *isolate = (ContextGroup*) group;
    isolate->release();
}

NATIVE(JSContext,jlong,create) (PARAMS) {
    long out;
    ContextGroup *group = new ContextGroup();
    {
        V8_ISOLATE(group->isolate());

        JSContext *ctx = new JSContext(group, Context::New(group->isolate()));
        out = reinterpret_cast<long>(ctx);
    }

    group->release();

    return out;
}

NATIVE(JSContext,jlong,createInGroup) (PARAMS,jlong grp) {
    long out;
    ContextGroup *group = reinterpret_cast<ContextGroup*>(grp);
    {
        V8_ISOLATE(group->isolate());

        JSContext *ctx = new JSContext(group, Context::New(group->isolate()));
        out = reinterpret_cast<long>(ctx);
    }

    return out;
}

NATIVE(JSContext,jlong,retain) (PARAMS,jlong ctx) {
    JSContext *context = reinterpret_cast<JSContext*>(ctx);
    context->retain();
    return ctx;
}

NATIVE(JSContext,void,release) (PARAMS,jlong ctx) {
    JSContext *context = reinterpret_cast<JSContext*>(ctx);
    context->release();
}

NATIVE(JSContext,jlong,getGlobalObject) (PARAMS, jlong ctx) {
    V8_ISOLATE_CTX(ctx,isolate,Ctx);
    return reinterpret_cast<long>(context_->Global());
}

NATIVE(JSContext,jlong,getGroup) (PARAMS, jlong ctx) {
    JSContext *context = reinterpret_cast<JSContext*>(ctx);
    context->Group()->retain();
    return reinterpret_cast<long>(context->Group());
}

NATIVE(JSContext,jobject,evaluateScript) (PARAMS, jlong ctx, jstring script,
        jstring sourceURL, jint startingLineNumber) {

    const char *_script = env->GetStringUTFChars(script, NULL);
    const char *_sourceURL = env->GetStringUTFChars(sourceURL, NULL);

    jclass ret = env->FindClass("org/liquidplayer/v8/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    jfieldID fid = env->GetFieldID(ret , "reference", "J");

    {
        JSContext *context_ = reinterpret_cast<JSContext*>(ctx);
        Isolate * isolate = context_->isolate();
        V8_ISOLATE(isolate);

        TryCatch trycatch;
        Local<Context> context;

        context = context_->Value();
        Context::Scope context_scope_(context);

        JSValue<Value> *exception = nullptr;

        ScriptOrigin script_origin(
            String::NewFromUtf8(isolate, _sourceURL, NewStringType::kNormal).ToLocalChecked(),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<String> source = String::NewFromUtf8(isolate, _script, NewStringType::kNormal);
        MaybeLocal<Script> script;

        MaybeLocal<Value>  result;
        if (source.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!exception) {
            script = Script::Compile(context, source.ToLocalChecked(), &script_origin);
            if (script.IsEmpty()) {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!exception) {
            JSValue<Value> *value = JSValue<Value>::New(context_, result.ToLocalChecked());
            env->SetLongField( out, fid, reinterpret_cast<long>(value));
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField(out, fid, reinterpret_cast<long>(exception));
    }

    env->ReleaseStringUTFChars(script, _script);
    env->ReleaseStringUTFChars(sourceURL, _sourceURL);

    return out;
}

