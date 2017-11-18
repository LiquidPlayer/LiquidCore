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

#include "JSJNI.h"
#include "../JSC/JSC.h"

NATIVE(JSContext,void,runInContextGroup) (PARAMS, jlong ctxGroup, jobject runnable) {
    ContextGroup *group = reinterpret_cast<ContextGroup*>(ctxGroup);

    if (group && group->Loop() && std::this_thread::get_id() != group->Thread()) {
        group->m_async_mutex.lock();

        struct Runnable *r = new struct Runnable;
        r->thiz = env->NewGlobalRef(thiz);
        r->runnable = env->NewGlobalRef(runnable);
        r->c_runnable = nullptr;
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
                __android_log_assert("FAIL", "runInContextGroup",
                    "Internal error.  Can't call back.");
                return;
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        env->CallVoidMethod(thiz, mid, runnable);
    }
}

NATIVE(JSContextGroup,jlong,create) (PARAMS) {
    // Maintain compatibility at the ContextGroup level with JSC
    const ContextGroup *group = JSContextGroupCreate();
    return reinterpret_cast<long>(group);
}

NATIVE(JSContextGroup,void,release) (PARAMS,jlong group) {
    ContextGroup *isolate = (ContextGroup*) group;
#ifdef DEBUG_RETAINER
    Retainer::m_debug_mutex.lock();
    bool found = (std::find(Retainer::m_debug.begin(),
        Retainer::m_debug.end(), isolate) != Retainer::m_debug.end());
    Retainer::m_debug_mutex.unlock();
    if (!found) {
        __android_log_assert("FAIL", "ContextGroup::release",
            "Attempting to release a deleted object!");
    }
#endif
    isolate->release();
}

NATIVE(JSContextGroup,jboolean,isManaged) (PARAMS, jlong ctxGroup) {
    ContextGroup *group = reinterpret_cast<ContextGroup*>(ctxGroup);
    return group && group->Loop();
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
        V8_ISOLATE(group,isolate)
            JSContext *ctx = new JSContext(group, Context::New(isolate));
            out = reinterpret_cast<long>(ctx);
        V8_UNLOCK()
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
#ifdef DEBUG_RETAINER
    Retainer::m_debug_mutex.lock();
    bool found = (std::find(Retainer::m_debug.begin(),
        Retainer::m_debug.end(), context) != Retainer::m_debug.end());
    Retainer::m_debug_mutex.unlock();
    if (!found) {
        __android_log_assert("FAIL", "JSContext::release",
            "Attempting to release a deleted object!");
    }
#endif
    context->release();
}

NATIVE(JSContext,jlong,getGlobalObject) (PARAMS, jlong ctx) {
    jlong v=0;

    V8_ISOLATE_CTX(ctx,isolate,Ctx)
        v = reinterpret_cast<long>(context_->Global());
    V8_UNLOCK()

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

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    jfieldID fid = env->GetFieldID(ret , "reference", "J");

    {
        JSContext *context_ = reinterpret_cast<JSContext*>(ctx);
        V8_ISOLATE(context_->Group(), isolate)
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
        V8_UNLOCK()
    }

    env->ReleaseStringUTFChars(script, _script);
    env->ReleaseStringUTFChars(sourceURL, _sourceURL);

    return out;
}
