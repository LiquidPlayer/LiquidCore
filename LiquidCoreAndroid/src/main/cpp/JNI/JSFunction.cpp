/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <cstdlib>
#include <boost/make_shared.hpp>
#include "JNI/JNI.h"
#include "JNI/JSFunction.h"

using namespace v8;

JSFunction::JSFunction(JNIEnv* env, jobject thiz, boost::shared_ptr<JSContext> ctx, jstring name_)
{
    m_context = ctx;
    m_isUndefined = false;
    m_isNull = false;
    m_wrapped = true;
    m_isObject = true;
    m_isNumber = false;
    m_isBoolean = false;

    env->GetJavaVM(&m_jvm);
    m_JavaThis = env->NewWeakGlobalRef(thiz);

    auto getMid = [&](const char* cb, const char *signature) -> jmethodID {
        jmethodID mid;
        jclass cls = env->GetObjectClass(thiz);
        do {
            mid = env->GetMethodID(cls,cb,signature);
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == nullptr || env->ExceptionCheck()) {
                if (super != nullptr) env->DeleteLocalRef(super);
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
    const char *c_string = env->GetStringUTFChars(name_, nullptr);

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

        m_value.Reset(isolate, function);
    V8_UNLOCK()

    env->ReleaseStringUTFChars(name_, c_string);
}

boost::shared_ptr<JSValue> JSFunction::New(JNIEnv* env, jobject thiz,
                                           jlong javaContext, jstring name_)
{
    auto ctx = SharedWrap<JSContext>::Shared(javaContext);
    auto p = boost::make_shared<JSFunction>(env, thiz, ctx, name_);

    // Retain a reference to Java functions for the lifetime of the context
    ctx->retain(p);

    ctx->Group()->Manage(p);
    return p;
}

JSFunction::~JSFunction() = default;

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
    int argumentCount = info.Length();
    jlong args[argumentCount];

    int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&env, nullptr);
    }

    Isolate *isolate = info.GetIsolate();
    HandleScope handle_scope(isolate);
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
        info.GetReturnValue().Set(info.This());
    } else {
        jlong retval = env->CallLongMethod(m_JavaThis, m_functionMid, objThis, argsArr);
        info.GetReturnValue().Set(
                SharedWrap<JSValue>::Shared(ctxt, retval)->Value()
        );
    }

    env->DeleteLocalRef(argsArr);

    boost::shared_ptr<JSValue> exception = m_exception;
    if (exception) {
        Local<v8::Value> excp = exception->Value();
        isolate->ThrowException(excp);
    }

    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->DetachCurrentThread();
    }
}
