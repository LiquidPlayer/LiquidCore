//
// JSFunction.cpp
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
 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

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
#include <cstdlib>
#include <boost/make_shared.hpp>
#include "JNI/JNI.h"
#include "JNI/JSFunction.h"

using namespace v8;

JSFunction::JSFunction(JNIEnv* env, jobject thiz, boost::shared_ptr<JSContext> ctx, jstring name_)
{
    env->GetJavaVM(&m_jvm);
    m_JavaThis = env->NewWeakGlobalRef(thiz);

    Persistent<v8::Value, CopyablePersistentTraits<v8::Value>> value;
    auto getMid = [&](const char* cb, const char *signature) -> jmethodID {
        jmethodID mid;
        jclass cls = env->GetObjectClass(thiz);
        do {
            mid = env->GetMethodID(cls,cb,signature);
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == NULL || env->ExceptionCheck()) {
                if (super != NULL) env->DeleteLocalRef(super);
                __android_log_assert("FAIL", "FunctionCallback",
                                     "Did not find callback method");
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);
        return mid;
    };

    m_constructorMid = getMid("constructorCallback","(J[J)V");
    m_functionMid = getMid("functionCallback","(J[J)J");
    const char *c_string = env->GetStringUTFChars(name_, NULL);

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<v8::Value> data = Wrap(this);

        Local<String> name =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();

        Local<FunctionTemplate> ctor =
            FunctionTemplate::New(isolate, StaticFunctionCallback, data);
        Local<Function> function = ctor->GetFunction();
        function->SetName(name);

        Local<Private> privateKey = v8::Private::ForApi(isolate,
            String::NewFromUtf8(isolate, "__JSValue_ptr"));
        function->SetPrivate(context, privateKey, data);
        m_wrapped = true;

        value = Persistent<v8::Value,CopyablePersistentTraits<v8::Value>>(isolate, function);
    V8_UNLOCK()

    env->ReleaseStringUTFChars(name_, c_string);

    m_isNull = false;
    m_isUndefined = false;
    m_value = value;
    m_context = ctx;
}

boost::shared_ptr<JSValue> JSFunction::New(JNIEnv* env, jobject thiz, jlong javaContext, jstring name_)
{
    auto ctx = SharedWrap<JSContext>::Shared(javaContext);
    auto p = boost::make_shared<JSFunction>(env, thiz, ctx, name_);
    ctx->retain(p);
    ctx->Group()->Manage(p);
    return p;
}

JSFunction::~JSFunction()
{
}

void JSFunction::StaticFunctionCallback(const FunctionCallbackInfo< v8::Value > &info)
{
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());

    JSFunction *this_ = static_cast<JSFunction*>(Unwrap(info.Data()));
    this_->FunctionCallback(info);
}

void JSFunction::FunctionCallback(const FunctionCallbackInfo< v8::Value > &info)
{
    JNIEnv *env;
    jlong objThis = 0;
    jlongArray argsArr = nullptr;
    bool isConstructCall = info.IsConstructCall();
    jlong retval = 0;
    int argumentCount = info.Length();
    jlong args[argumentCount];

    int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&env, NULL);
    }
    Isolate *isolate = info.GetIsolate();

    boost::shared_ptr<JSContext> ctxt = m_context;
    Local<v8::Context> context = ctxt->Value();
    Context::Scope context_scope_(context);

    objThis = SharedWrap<JSValue>::New(JSValue::New(ctxt, info.This()));

    argsArr = env->NewLongArray(argumentCount);
    for (int i=0; i<argumentCount; i++) {
        args[i] = SharedWrap<JSValue>::New(JSValue::New(ctxt, info[i]));
    }
    env->SetLongArrayRegion(argsArr,0,argumentCount,args);

    clearException();
    if (isConstructCall) {
        env->CallVoidMethod(m_JavaThis, m_constructorMid, objThis, argsArr);
    } else {
        retval = env->CallLongMethod(m_JavaThis, m_functionMid, objThis, argsArr);
    }

    env->DeleteLocalRef(argsArr);

    if (isConstructCall) {
        info.GetReturnValue().Set(info.This());
    } else {
        if (retval) {
            info.GetReturnValue().Set(
                SharedWrap<JSValue>::Shared(retval)->Value()
            );
        } else {
            info.GetReturnValue().SetUndefined();
        }
    }

    boost::shared_ptr<JSValue> exception = m_exception;
    if (exception) {
        Local<v8::Value> excp = exception->Value();
        isolate->ThrowException(excp);
    }

    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->DetachCurrentThread();
    }
}
