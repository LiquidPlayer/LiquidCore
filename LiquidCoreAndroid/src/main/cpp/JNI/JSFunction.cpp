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
#include "JNI/JNIReturnObject.h"

using namespace v8;

#define JSR "Lorg/liquidplayer/javascript/JNIReturnObject;"

JSFunction::JSFunction(JNIEnv* env, jobject thiz, boost::shared_ptr<JSContext> ctx, jstring name_)
{
    env->GetJavaVM(&m_jvm);
    m_JavaThis = env->NewWeakGlobalRef(thiz);

    Persistent<v8::Value, CopyablePersistentTraits<v8::Value>> value;

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<v8::Value> data = Wrap(this);

        const char *c_string = env->GetStringUTFChars(name_, NULL);
        Local<String> name =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(name_, c_string);

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

    if (!s_jnijsvalue_class)
    s_jnijsvalue_class = (jclass) env->NewGlobalRef(findClass(env, "org/liquidplayer/javascript/JNIJSValue"));
    m_constructorMid = 0;
    m_functionMid = 0;

    m_isNull = false;
    m_isUndefined = false;
    m_value = value;
    m_context = ctx;
}

jclass JSFunction::s_jnijsvalue_class = 0;

jmethodID JSFunction::getMethodId(JNIEnv *env, bool isContructCall)
{
    auto getMid = [&](const char* cb, const char *signature) -> jmethodID {
        jmethodID mid;
        jclass cls = env->GetObjectClass(m_JavaThis);
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

    if (isContructCall && !m_constructorMid)
        m_constructorMid = getMid("constructorCallback","(J[J)" JSR);
    else if (!isContructCall && !m_functionMid)
        m_functionMid = getMid("functionCallback","(J[J)" JSR);

    return (isContructCall) ? m_constructorMid : m_functionMid;
}

boost::shared_ptr<JSValue> JSFunction::New(JNIEnv* env, jobject thiz, jlong javaContext, jstring name_)
{
    auto ctx = SharedWrap<JSContext>::Shared(env, javaContext);
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
    jlong objThis = 0;
    jlongArray argsArr = nullptr;
    jlong *args = nullptr;
    bool isConstructCall = info.IsConstructCall();
    jobject objret = 0;
    int argumentCount = info.Length();

    JNIEnv *env;
    int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&env, NULL);
    }

    jmethodID mid = getMethodId(env, isConstructCall);

    boost::shared_ptr<JSContext> ctxt = m_context;
    boost::shared_ptr<ContextGroup> grp;
    if (ctxt) {
        grp = ctxt->Group();

        V8_ISOLATE(grp, isolate)
            Local<v8::Context> context = ctxt->Value();
            Context::Scope context_scope_(context);

            objThis = SharedWrap<JSValue>::New(
                env,
                JSValue::New(ctxt, info.This())
            );

            argsArr = env->NewLongArray(argumentCount);
            args = new jlong[argumentCount];
            for (int i=0; i<argumentCount; i++) {
                args[i] = SharedWrap<JSValue>::New(
                    env,
                    JSValue::New(ctxt, info[i])
                );

                /*
                env->SetObjectArrayElement(
                    argsArr,
                    i,
                    args[i]
                );
                */
            }
        env->SetLongArrayRegion(argsArr,0,argumentCount,args);
        V8_UNLOCK()
    }

    objret = env->CallObjectMethod(m_JavaThis, mid, objThis, argsArr);

    env->DeleteLocalRef(argsArr);
    //env->DeleteLocalRef(objThis);
    /*
    for (int i=0; i<argumentCount; i++) {
        env->DeleteLocalRef(args[i]);
    }
    */
    delete [] args;

    V8_ISOLATE(grp, isolate)
        JNIReturnObject ret(env, objret);
        Local<v8::Context> context = ctxt->Value();
        Context::Scope context_scope_(context);

        if (isConstructCall) {
            info.GetReturnValue().Set(info.This());
        } else {
            jlong retval = ret.GetReference();
            if (retval) {
                info.GetReturnValue().Set(
                    SharedWrap<JSValue>::Shared(
                        env,
                        retval
                    )->Value()
                );
                //env->DeleteLocalRef(retval);
            } else {
                info.GetReturnValue().SetUndefined();
            }
        }

        if (ret.GetException()) {
            auto exception = SharedWrap<JSValue>::Shared(env, ret.GetException());
            Local<v8::Value> excp = exception->Value();
            isolate->ThrowException(excp);
        }
    V8_UNLOCK()

    env->DeleteLocalRef(objret);

    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->DetachCurrentThread();
    }
}
