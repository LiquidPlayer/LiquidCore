/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
