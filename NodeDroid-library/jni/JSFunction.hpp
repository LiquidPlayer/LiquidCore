//
// JSFunction.h
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

#ifndef ANDROIDJSCORE_JSFUNCTION_H
#define ANDROIDJSCORE_JSFUNCTION_H

#include "JSJNI.h"

template <typename T>
class JSFunction : public JSValue<T> {
    public:
        JSFunction(JNIEnv* env, jobject thiz, jlong ctx, jstring name_) {
            env->GetJavaVM(&m_jvm);
            m_JavaThis = env->NewWeakGlobalRef(thiz);

            V8_ISOLATE_CTX(ctx,isolate,context);
            long this_ = reinterpret_cast<long>(this);

            const char *c_string = env->GetStringUTFChars(name_, NULL);
            Local<String> name =
                String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
            env->ReleaseStringUTFChars(name_, c_string);

            Local<Value> data = Number::New(isolate, this_);

            Local<FunctionTemplate> ctor =
                FunctionTemplate::New(isolate, StaticFunctionCallback, data);
            Local<Function> function = ctor->GetFunction();
            function->SetName(name);

            function->SetHiddenValue(String::NewFromUtf8(isolate, "__JSValue_ptr"),
                Number::New(isolate,(double)reinterpret_cast<long>(this)));

            JSValue<T>::m_isNull = false;
            JSValue<T>::m_isUndefined = false;
            JSValue<T>::m_value = Persistent<T,CopyablePersistentTraits<T>>(isolate, function);
            JSValue<T>::m_context = context_;
            JSValue<T>::m_context->retain();
            Retainer::m_count = 1;
        }

    protected:
        virtual ~JSFunction() {
        }

    private:
        static void StaticFunctionCallback(const FunctionCallbackInfo< Value > &info) {
            Isolate::Scope isolate_scope_(info.GetIsolate());
            HandleScope handle_scope_(info.GetIsolate());

            JSFunction<T> *this_ = reinterpret_cast<JSFunction*>(
                (long)(info.Data()->ToNumber()->Value()));
            this_->FunctionCallback(info);
        }

        void FunctionCallback(const FunctionCallbackInfo< Value > &info) {
            Isolate *isolate = JSValue<T>::isolate();
            V8_ISOLATE(isolate);
            Local<Context> context = JSValue<T>::m_context->Value();
            Context::Scope context_scope_(context);

            JNIEnv *env;
            int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (getEnvStat == JNI_EDETACHED) {
                m_jvm->AttachCurrentThread(&env, NULL);
            }

            jclass cls = env->GetObjectClass(m_JavaThis);
            jmethodID mid;
            do {
                if (info.IsConstructCall()) {
                    mid = env->GetMethodID(cls,"constructorCallback","(J[JJ)V");
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
                        m_jvm->DetachCurrentThread();
                    }
                    return;
                }
                cls = super;
            } while (true);
            env->DeleteLocalRef(cls);

            JSValue<Value> * jsThis = JSValue<Value>::New(JSValue<T>::m_context, info.This());

            int argumentCount = info.Length();

            jlongArray argsArr = env->NewLongArray(argumentCount);
            jlong *args = new jlong[argumentCount];
            for (size_t i=0; i<argumentCount; i++) {
                args[i] = reinterpret_cast<long>(JSValue<Value>::New(
                    JSValue<T>::m_context, info[i]));
            }
            env->SetLongArrayRegion(argsArr,0,argumentCount,args);

            JSValue<Value> *exception = nullptr;
            jlong exceptionRefRef = reinterpret_cast<long>(&exception);
            jlong objThis = reinterpret_cast<long>(jsThis);

            if (info.IsConstructCall()) {
                env->CallVoidMethod(m_JavaThis, mid, objThis, argsArr, exceptionRefRef);
                info.GetReturnValue().Set(info.This());
            } else {
                jlong objret =
                    env->CallLongMethod(m_JavaThis, mid, objThis, argsArr, exceptionRefRef);
                if (objret) {
                    info.GetReturnValue().Set(reinterpret_cast<JSValue<Value>*>(objret)->Value());
                } else {
                    info.GetReturnValue().SetUndefined();
                }
            }

            //jsThis->release();
            //jsThis = nullptr;

            if (exception) {
                __android_log_write(ANDROID_LOG_DEBUG, "FunctionCallback", "Yeah there was an exception");
                isolate->ThrowException(exception->Value());
            }

            delete args;
            env->DeleteLocalRef(argsArr);

            if (getEnvStat == JNI_EDETACHED) {
                m_jvm->DetachCurrentThread();
            }
        }

        JavaVM *m_jvm;
        jobject m_JavaThis;
};

#endif //ANDROIDJSCORE_JSFUNCTION_H
