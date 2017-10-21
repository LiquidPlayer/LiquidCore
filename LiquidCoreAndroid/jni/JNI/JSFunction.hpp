//
// JSFunction.h
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

            Persistent<T, CopyablePersistentTraits<T>> value;

            V8_ISOLATE_CTX(ctx,isolate,context)
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

                Local<Private> privateKey = v8::Private::ForApi(isolate,
                    String::NewFromUtf8(isolate, "__JSValue_ptr"));
                function->SetPrivate(context, privateKey,
                    Number::New(isolate,(double)reinterpret_cast<long>(this)));

                value = Persistent<T,CopyablePersistentTraits<T>>(isolate, function);
            V8_UNLOCK()

            JSValue<T>::m_isNull = false;
            JSValue<T>::m_isUndefined = false;
            JSValue<T>::m_value = value;
            JSValue<T>::m_context = context_;
            JSValue<T>::m_context->retain(this);
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
                (long)(info.Data()->ToNumber(info.GetIsolate())->Value()));
            this_->FunctionCallback(info);
        }

        void FunctionCallback(const FunctionCallbackInfo< Value > &info) {
            jlong objThis = 0L;
            jlongArray argsArr = nullptr;
            jlong *args = nullptr;
            jmethodID mid;
            JSValue<Value> *exception = nullptr;
            jlong exceptionRefRef = reinterpret_cast<jlong>(&exception);
            jlong objret = 0;
            bool isConstructCall = false;

            JNIEnv *env;
            int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (getEnvStat == JNI_EDETACHED) {
                m_jvm->AttachCurrentThread(&env, NULL);
            }

            ContextGroup *grp = JSValue<T>::m_context->Group();
            JSContext *ctxt = JSValue<T>::m_context;

            {
            V8_ISOLATE(grp, isolate)
                Local<Context> context = ctxt->Value();
                Context::Scope context_scope_(context);

                isConstructCall = info.IsConstructCall();

                jclass cls = env->GetObjectClass(m_JavaThis);
                do {
                    if (isConstructCall) {
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
                        // FIXME: We should assert something here
                        return; // Drops out of this context
                    }
                    cls = super;
                } while (true);
                env->DeleteLocalRef(cls);

                objThis = reinterpret_cast<jlong>
                    (JSValue<Value>::New(ctxt, info.This()));

                int argumentCount = info.Length();

                argsArr = env->NewLongArray(argumentCount);
                args = new jlong[argumentCount];
                for (int i=0; i<argumentCount; i++) {
                    args[i] = reinterpret_cast<long>(JSValue<Value>::New(ctxt, info[i]));
                }
                env->SetLongArrayRegion(argsArr,0,argumentCount,args);
            V8_UNLOCK()
            }

            // The 'return' statement above only returns from the lambda function buried
            // in the V8* macro
            if (!args) return;

            if (isConstructCall) {
                env->CallVoidMethod(m_JavaThis, mid, objThis, argsArr, exceptionRefRef);
            } else {
                objret =
                    env->CallLongMethod(m_JavaThis, mid, objThis, argsArr, exceptionRefRef);
            }

            V8_ISOLATE(grp, isolate)
                Local<Context> context = ctxt->Value();
                Context::Scope context_scope_(context);

                if (isConstructCall) {
                    info.GetReturnValue().Set(info.This());
                } else {
                    if (objret) {
                        info.GetReturnValue().Set(reinterpret_cast<JSValue<Value>*>(objret)->Value());
                    } else {
                        info.GetReturnValue().SetUndefined();
                    }
                }

                if (exception) {
                    Local<Value> excp = exception->Value();
                    exception->release();
                    isolate->ThrowException(excp);
                }
            V8_UNLOCK()

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
