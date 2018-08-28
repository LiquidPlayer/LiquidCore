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

#include "JNI/JNI.h"
#include "JSC/JSC.h"
#include "JNIJSException.h"

extern "C" jlong Java_org_liquidplayer_javascript_JNIJSContextGroup_create(JNIEnv *, jobject);

NATIVE(JNIJSContext,jlong,createInGroup) (PARAMS,jlong grp)
{
    auto group = SharedWrap<ContextGroup>::Shared(grp);
    jlong ctx;
    { V8_ISOLATE(group,isolate)
        ctx = SharedWrap<JSContext>::New(JSContext::New(group, Context::New(isolate)));
    V8_UNLOCK() }

    return ctx;
}

NATIVE(JNIJSContext,jlong,create) (PARAMS)
{
    jlong jGroup = Java_org_liquidplayer_javascript_JNIJSContextGroup_create(env, thiz);
    return Java_org_liquidplayer_javascript_JNIJSContext_createInGroup(env, thiz, jGroup);
}

NATIVE(JNIJSContext,void,Finalize) (PARAMS, jlong reference)
{
    SharedWrap<JSContext>::Dispose(reference);
}

NATIVE(JNIJSContext,jlong,getGlobalObject) (PARAMS, jlong ctxRef)
{
    jlong v=0;
    auto ctx = SharedWrap<JSContext>::Shared(ctxRef);

    V8_ISOLATE_CTX(ctx,isolate,Ctx)
        v = SharedWrap<JSValue>::New(ctx->Global());
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSContext,jlong,getGroup) (PARAMS, jlong grpRef)
{
    auto context = SharedWrap<JSContext>::Shared(grpRef);
    return SharedWrap<ContextGroup>::New(context->Group());
}

NATIVE(JNIJSContext,jlong,evaluateScript) (PARAMS, jlong ctxRef, jstring script_,
    jstring sourceURL_, jint startingLineNumber)
{
    auto ctx = SharedWrap<JSContext>::Shared(ctxRef);

    const char *_script = env->GetStringUTFChars(script_, NULL);
    const char *_sourceURL = env->GetStringUTFChars(sourceURL_, NULL);

    jlong ret = 0;
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE(ctx->Group(), isolate)
        TryCatch trycatch(isolate);

        Local<Context> context = ctx->Value();
        Context::Scope context_scope_(context);

        ScriptOrigin script_origin(
            String::NewFromUtf8(isolate, _sourceURL, NewStringType::kNormal).ToLocalChecked(),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<String> source = String::NewFromUtf8(isolate, _script, NewStringType::kNormal);
        MaybeLocal<Script> script;

        MaybeLocal<Value>  result;
        if (source.IsEmpty()) {
            exception = JSValue::New(ctx, trycatch.Exception());
        }

        if (!exception) {
            script = Script::Compile(context, source.ToLocalChecked(), &script_origin);
            if (script.IsEmpty()) {
                exception = JSValue::New(ctx, trycatch.Exception());
            }
        }

        if (!exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                exception = JSValue::New(ctx, trycatch.Exception());
            }
        }

        if (!exception) {
            boost::shared_ptr<JSValue> value =
                JSValue::New(ctx, result.ToLocalChecked());
            ret = SharedWrap<JSValue>::New(value);
        }

    V8_UNLOCK()

    env->ReleaseStringUTFChars(script_, _script);
    env->ReleaseStringUTFChars(sourceURL_, _sourceURL);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return ret;
}
