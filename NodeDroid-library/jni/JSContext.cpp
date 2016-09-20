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

/**
 * class Retainer
 **/

#ifdef DEBUG_RETAINER
std::list<Retainer*> Retainer::m_debug;
std::mutex Retainer::m_debug_mutex;
#endif

/**
 * class JSValue<T>
 **/

template <typename T>
JSValue<T> * JSValue<T>::New(JSContext* context, Local<T> val) {
    if (val->IsObject()) {
        Local<Private> privateKey = v8::Private::ForApi(context->isolate(),
            String::NewFromUtf8(context->isolate(), "__JSValue_ptr"));
        Local<Object> obj = val->ToObject(context->Value()).ToLocalChecked();
        Local<v8::Value> identifier;
        Maybe<bool> result = obj->HasPrivate(context->Value(), privateKey);
        bool hasPrivate = false;
        if (result.IsJust() && result.FromJust()) {
            hasPrivate = obj->GetPrivate(context->Value(), privateKey).ToLocal(&identifier);
        }
        if (hasPrivate && identifier->IsNumber()) {
            // This object is already wrapped, let's re-use it
            JSValue<T> *value =
                reinterpret_cast<JSValue<T>*>(
                    (long)identifier->ToNumber(context->Value()).ToLocalChecked()->Value());
            value->retain();
            return value;
        } else {
            // First time wrap.  Create it new and mark it
            JSValue<T> *value = new JSValue<T>(context,val);
            obj->SetPrivate(context->Value(), privateKey,
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
    m_context->retain(this);
}

template <typename T>
JSValue<T>::~JSValue<T>() {
    V8_ISOLATE(m_context->Group(), isolate);

    if (!m_isUndefined && !m_isNull) {
        if (Value()->IsObject()) {
            Local<Object> obj = Value()->ToObject(m_context->Value()).ToLocalChecked();
            // Clear wrapper pointer if it exists, in case this object is still held by JS
            Local<Private> privateKey = v8::Private::ForApi(isolate,
                String::NewFromUtf8(isolate, "__JSValue_ptr"));
            obj->SetPrivate(m_context->Value(), privateKey,
                Local<v8::Value>::New(isolate,Undefined(isolate)));
        }
        m_value.Reset();
    }

    m_context->release(this);
    V8_UNLOCK();
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

/**
 * class JSContext
 **/

JSContext::JSContext(ContextGroup* isolate, Local<Context> val) {
    m_isolate = isolate;
    m_isolate->retain();
    m_context = Persistent<Context,CopyablePersistentTraits<Context>>(isolate->isolate(), val);
    m_isDefunct = false;
}

JSContext::~JSContext() {
    m_context.Reset();

    m_isolate->release();
};

void JSContext::SetDefunct() {
    m_isDefunct = true;
    while (m_value_set.size() > 0) {
        std::set<JSValue<v8::Value>*>::iterator it = m_value_set.begin();
        (*it)->release();
    }

    while (m_object_set.size() > 0) {
        std::set<JSValue<v8::Object>*>::iterator it = m_object_set.begin();
        (*it)->release();
    }
}

void JSContext::retain(JSValue<v8::Value>* value) {
    Retainer::retain();
    m_value_set.insert(value);
}

void JSContext::release(JSValue<v8::Value>* value) {
    m_value_set.erase(value);
    Retainer::release();
}

void JSContext::retain(JSValue<v8::Object>* value) {
    Retainer::retain();
    m_object_set.insert(value);
}

void JSContext::release(JSValue<v8::Object>* value) {
    m_object_set.erase(value);
    Retainer::release();
}

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

/**
 * class ContextGroup
 **/

Platform *ContextGroup::s_platform = NULL;
int ContextGroup::s_init_count = 0;
std::mutex ContextGroup::s_mutex;

void ContextGroup::init_v8() {
    s_mutex.lock();
    if (s_init_count++ == 0) {
        s_platform = platform::CreateDefaultPlatform(4);
        V8::InitializePlatform(s_platform);
        V8::Initialize();
    }
    s_mutex.unlock();
}

void ContextGroup::dispose_v8() {
    s_mutex.lock();
    // FIXME: Once disposed, an attempt to re-init will crash
    // For now, init once and never dispose
    //--s_init_count;
    if (s_init_count == 0) {
        V8::Dispose();
        V8::ShutdownPlatform();
        delete s_platform;
        s_platform = nullptr;
    }
    s_mutex.unlock();
}

ContextGroup::ContextGroup() {
    init_v8();
    m_create_params.array_buffer_allocator = &m_allocator;
    m_isolate = Isolate::New(m_create_params);
    m_manage_isolate = true;
    m_uv_loop = nullptr;
    m_thread_id = std::this_thread::get_id();
    m_async_handle = nullptr;
}

ContextGroup::ContextGroup(Isolate *isolate, uv_loop_t *uv_loop) {
    m_isolate = isolate;
    m_manage_isolate = false;
    m_uv_loop = uv_loop;
    m_thread_id = std::this_thread::get_id();
    m_async_handle = nullptr;
}

void ContextGroup::callback(uv_async_t* handle) {
    ContextGroup *group = reinterpret_cast<ContextGroup*>(handle->data);

    group->m_async_mutex.lock();
    struct Runnable *r = group->m_runnables.empty() ? nullptr : group->m_runnables.front();

    while (r) {
        group->m_async_mutex.unlock();

        JNIEnv *env;
        int getEnvStat = r->jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            r->jvm->AttachCurrentThread(&env, NULL);
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
                    r->jvm->DetachCurrentThread();
                }
                __android_log_assert("FAIL", "ContextGroup::callback",
                    "Can't find the class to call back?");
                break;
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        env->CallVoidMethod(r->thiz, mid, r->runnable);

        env->DeleteGlobalRef(r->thiz);
        env->DeleteGlobalRef(r->runnable);

        if (getEnvStat == JNI_EDETACHED) {
            r->jvm->DetachCurrentThread();
        }

        group->m_async_mutex.lock();

        group->m_runnables.erase(group->m_runnables.begin());
        delete r;

        r = group->m_runnables.empty() ? nullptr : group->m_runnables.front();
    }
    // Close the handle.  We will create a new one if we
    // need another.  This keeps the node process from staying alive
    // indefinitely
    uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
        delete (uv_async_t*)h;
    });
    group->m_async_handle = nullptr;
    group->m_async_mutex.unlock();
}

ContextGroup::~ContextGroup() {
    if (m_manage_isolate) {
        // Occasionally, the finalizer will run and attempt to dispose of an isolate
        // before another thread has finished with it.  To avoid the message below,
        // we dutifully wait for our turn to enter the isolate before disposing.
        //#
        //# Fatal error in v8::Isolate::Dispose()
        //# Disposing the isolate that is entered by a thread.
        //#
        {
            V8_ISOLATE(this,isolate);
            V8_UNLOCK();
        }
        m_isolate->Dispose();
    }
    dispose_v8();
}

/**
 * JSContext JNI Wrappers
 **/

NATIVE(JSContext,void,runInContextGroup) (PARAMS, jlong ctxGroup, jobject runnable) {
    ContextGroup *group = reinterpret_cast<ContextGroup*>(ctxGroup);

    if (group && group->Loop() && std::this_thread::get_id() != group->Thread()) {
        group->m_async_mutex.lock();

        struct Runnable *r = new struct Runnable;
        r->thiz = env->NewGlobalRef(thiz);
        r->runnable = env->NewGlobalRef(runnable);
        env->GetJavaVM(&r->jvm);
        group->m_runnables.push_back(r);

        if (!group->m_async_handle) {
            group->m_async_handle = new uv_async_t();
            group->m_async_handle->data = group;
            uv_async_init(group->Loop(), group->m_async_handle, ContextGroup::callback);
            uv_async_send(group->m_async_handle);
        }
        group->m_async_mutex.unlock();
    } else {
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
        V8_ISOLATE(group,isolate);

        JSContext *ctx = new JSContext(group, Context::New(isolate));
        out = reinterpret_cast<long>(ctx);

        V8_UNLOCK();
    }

    group->release();

    return out;
}

NATIVE(JSContext,jlong,createInGroup) (PARAMS,jlong grp) {
    long out;
    ContextGroup *group = reinterpret_cast<ContextGroup*>(grp);
    {
        V8_ISOLATE(group,isolate);

        JSContext *ctx = new JSContext(group, Context::New(isolate));
        out = reinterpret_cast<long>(ctx);

        V8_UNLOCK();
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
    jlong v = reinterpret_cast<long>(context_->Global());
    V8_UNLOCK();
    return v;
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
        V8_ISOLATE(context_->Group(), isolate);

        TryCatch trycatch(isolate);
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

        V8_UNLOCK();
    }

    env->ReleaseStringUTFChars(script, _script);
    env->ReleaseStringUTFChars(sourceURL, _sourceURL);

    return out;
}

