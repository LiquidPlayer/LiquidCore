//
// JSFunction.cpp
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2016 Eric Lange. All rights reserved.

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

#include "JSFunction.h"

template <typename T>
JSFunction<T>::JSFunction(JNIEnv* env, jobject thiz, jlong ctx, jstring name_) : JSValue<T>()
{
    env->GetJavaVM(&jvm);
    this->thiz = env->NewWeakGlobalRef(thiz);

    V8_ISOLATE_CTX(ctx,isolate,context);
    long this_ = reinterpret_cast<long>(this);

    const char *c_string = env->GetStringUTFChars(name_, NULL);
    Local<String> name =
        String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
    env->ReleaseStringUTFChars(name_, c_string);

    Local<Value> data = Number::New(isolate, this_);
    Local<Function> function = Function::New(context, StaticFunctionCallback, data)
        .ToLocalChecked();
    function->SetName(name);

    JSValue<T>::m_value = Persistent<T,CopyablePersistentTraits<T>>(isolate, function);
    JSValue<T>::m_isolate = context_;
    JSValue<T>::m_isolate->retain();

//    m_context = context_;
//    m_context->retain();
}

template <typename T>
JSFunction<T>::~JSFunction() {
//    m_function->release();
//    m_context->release();
}

template <typename T>
void JSFunction<T>::StaticFunctionCallback(const FunctionCallbackInfo< Value > &info) {
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());

    JSFunction *this_ = reinterpret_cast<JSFunction*>((long)(info.Data()->ToNumber()->Value()));
    this_->FunctionCallback(info);
}

template <typename T>
void JSFunction<T>::FunctionCallback(const FunctionCallbackInfo< Value > &info)
{
    Isolate *isolate = JSValue<T>::m_isolate->isolate();
    Local<Context> context = JSValue<T>::m_isolate->Value();
    Context::Scope context_scope_(context);

    JNIEnv *env;
    int getEnvStat = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&env, NULL);
    }

    jclass cls = env->GetObjectClass(thiz);
    jmethodID mid;
    do {
        if (info.IsConstructCall()) {
            mid = env->GetMethodID(cls,"constructorCallback","([JJ)J");
        } else {
            mid = env->GetMethodID(cls,"functionCallback","(J[JJ)J");
        }
        if (!env->ExceptionCheck()) break;
        env->ExceptionClear();
        jclass super = env->GetSuperclass(cls);
        env->DeleteLocalRef(cls);
        if (super == NULL || env->ExceptionCheck()) {
            if (super != NULL) env->DeleteLocalRef(super);
            if (getEnvStat == JNI_EDETACHED) {
                jvm->DetachCurrentThread();
            }
            return;
        }
        cls = super;
    } while (true);
    env->DeleteLocalRef(cls);

    int argumentCount = info.Length();

    jlongArray argsArr = env->NewLongArray(argumentCount);
    jlong* args = new jlong[argumentCount];
    for (size_t i=0; i<argumentCount; i++) {
        args[i] = reinterpret_cast<long>(new JSValue<Value>(JSValue<T>::m_isolate->Group(), info[i]));
    }
    env->SetLongArrayRegion(argsArr,0,argumentCount,args);

    JSValue<Value> *exception = nullptr;
    long objret;

    if (info.IsConstructCall()) {
        objret = env->CallLongMethod(thiz, mid, argsArr, reinterpret_cast<long>(&exception));
    } else {
        objret = env->CallLongMethod(thiz, mid, new JSValue<Value>(JSValue<T>::m_isolate->Group(), info.This()),
            argsArr, reinterpret_cast<long>(&exception));
    }

    if (exception) {
        isolate->ThrowException(exception->Value());
    } else {
        info.GetReturnValue().Set(reinterpret_cast<JSValue<Value>*>(objret)->Value());
    }

    delete args;
    env->DeleteLocalRef(argsArr);

    if (getEnvStat == JNI_EDETACHED) {
        jvm->DetachCurrentThread();
    }
}

/* FIXME
bool JSFunction::HasInstanceCallback(JSContextRef ctx, JSObjectRef constructor,
        JSValueRef possibleInstance, JSValueRef* exception)
{
    JNIEnv *env;
    int getEnvStat = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&env, NULL);
    }
    jclass cls = env->GetObjectClass(thiz);
    jmethodID mid;
    do {
        mid = env->GetMethodID(cls,"hasInstanceCallback","(JJJJ)Z");
        if (!env->ExceptionCheck()) break;
        env->ExceptionClear();
        jclass super = env->GetSuperclass(cls);
        env->DeleteLocalRef(cls);
        if (super == NULL || env->ExceptionCheck()) {
            if (super != NULL) env->DeleteLocalRef(super);
            if (getEnvStat == JNI_EDETACHED) {
                jvm->DetachCurrentThread();
            }
            return NULL;
        }
        cls = super;
    } while (true);
    env->DeleteLocalRef(cls);

    bool ret = env->CallBooleanMethod(thiz, mid, (jlong)ctx, (jlong)constructor,
            (jlong)possibleInstance, (jlong)exception);

    if (getEnvStat == JNI_EDETACHED) {
        jvm->DetachCurrentThread();
    }
    return ret;
}
*/
