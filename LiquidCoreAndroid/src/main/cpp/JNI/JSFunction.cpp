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
#include "JNI/JNI.h"
#include "JNI/JSFunction.h"
#include "JNI/JNIReturnObject.h"

using namespace v8;

#define JSO "Lorg/liquidplayer/javascript/JNIJSObject;"
#define JSV "Lorg/liquidplayer/javascript/JNIJSValue;"
#define JSR "Lorg/liquidplayer/javascript/JNIReturnObject;"

JSFunction::JSFunction(JNIEnv* env, jobject thiz, std::shared_ptr<JSContext> ctx, jstring name_)
{
    env->GetJavaVM(&m_jvm);
    m_JavaThis = env->NewWeakGlobalRef(thiz);

    Persistent<v8::Value, CopyablePersistentTraits<v8::Value>> value;

    V8_ISOLATE_CTX(ctx,isolate,context)
        long this_ = reinterpret_cast<long>(this);

        const char *c_string = env->GetStringUTFChars(name_, NULL);
        Local<String> name =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(name_, c_string);

        Local<v8::Value> data = Number::New(isolate, this_);

        Local<FunctionTemplate> ctor =
            FunctionTemplate::New(isolate, StaticFunctionCallback, data);
        Local<Function> function = ctor->GetFunction();
        function->SetName(name);

        Local<Private> privateKey = v8::Private::ForApi(isolate,
            String::NewFromUtf8(isolate, "__JSValue_ptr"));
        function->SetPrivate(context, privateKey,
            Number::New(isolate,(double)this_));
        m_wrapped = true;

        value = Persistent<v8::Value,CopyablePersistentTraits<v8::Value>>(isolate, function);
    V8_UNLOCK()

    JSValue::m_isNull = false;
    JSValue::m_isUndefined = false;
    JSValue::m_value = value;
    JSValue::m_context = ctx;
}

std::shared_ptr<JSValue> JSFunction::New(JNIEnv* env, jobject thiz, jobject javaContext, jstring name_)
{
    auto ctx = SharedWrap<JSContext>::Shared(env, javaContext);
    auto p = std::make_shared<JSFunction>(env, thiz, ctx, name_);
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

    JSFunction *this_ = reinterpret_cast<JSFunction*>(
        (long)(info.Data()->ToNumber(info.GetIsolate())->Value()));
    this_->FunctionCallback(info);
}

void JSFunction::FunctionCallback(const FunctionCallbackInfo< v8::Value > &info)
{
    jobject objThis = nullptr;
    jobjectArray argsArr = nullptr;
    jobject *args = nullptr;
    jmethodID mid;
    jobject objret = nullptr;
    bool isConstructCall = false;

    JNIEnv *env;
    int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&env, NULL);
    }

    auto grp = JSValue::m_context->Group();
    auto ctxt = JSValue::m_context;

    {
    V8_ISOLATE(grp, isolate)
        Local<v8::Context> context = ctxt->Value();
        Context::Scope context_scope_(context);

        isConstructCall = info.IsConstructCall();

        jclass cls = env->GetObjectClass(m_JavaThis);
        do {
            if (isConstructCall) {
                mid = env->GetMethodID(cls,"constructorCallback","(" JSO "[" JSV ")" JSR);
            } else {
                mid = env->GetMethodID(cls,"functionCallback","(" JSV "[" JSV ")" JSR);
            }
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == NULL || env->ExceptionCheck()) {
                if (super != NULL) env->DeleteLocalRef(super);
                if (getEnvStat == JNI_EDETACHED) {
                    m_jvm->DetachCurrentThread();
                }
                __android_log_assert("FAIL", "FunctionCallback",
                    "Did not find callback method");
                return; // Drops out of this context
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        objThis = SharedWrap<JSValue>::New(
            env,
            JSValue::New(ctxt, info.This())
        );

        int argumentCount = info.Length();

        jclass claz = findClass(env, "org/liquidplayer/javascript/JNIJSValue");
        argsArr = env->NewObjectArray(argumentCount, claz, nullptr);
        args = new jobject[argumentCount];
        for (int i=0; i<argumentCount; i++) {
            env->SetObjectArrayElement(
                argsArr,
                i,
                SharedWrap<JSValue>::New(
                    env,
                    JSValue::New(ctxt, info[i])
                )
            );
        }
    V8_UNLOCK()
    }

    // The 'return' statement above only returns from the lambda function buried
    // in the V8* macro
    if (!args) return;

    objret = env->CallObjectMethod(m_JavaThis, mid, objThis, argsArr);
    JNIReturnObject ret(env, objret);

    V8_ISOLATE(grp, isolate)
        Local<v8::Context> context = ctxt->Value();
        Context::Scope context_scope_(context);

        if (isConstructCall) {
            info.GetReturnValue().Set(info.This());
        } else {
            if (ret.GetReference()) {
                info.GetReturnValue().Set(
                    SharedWrap<JSValue>::Shared(
                        env,
                        ret.GetReference()
                    )->Value()
                );
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

    delete args;
    env->DeleteLocalRef(argsArr);

    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->DetachCurrentThread();
    }
}
