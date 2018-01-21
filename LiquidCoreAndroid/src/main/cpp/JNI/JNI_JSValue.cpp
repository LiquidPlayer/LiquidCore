//
// JSValue.cpp
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
#include "JNI/JNIReturnObject.h"

template <typename F>
jboolean boolean_func(JNIEnv* env, jobject thiz, F&& lambda, bool defValue){
    auto valueRef = SharedWrap<JSValue>::Shared(env, thiz);
    if (valueRef && !valueRef->IsDefunct() && !valueRef->Context()->IsDefunct() &&
            !valueRef->Group()->IsDefunct()) {
        V8_ISOLATE_CTX(valueRef->Context(), isolate, context)
            Local<Value> value = valueRef->Value();
            defValue = lambda(value, context);
        V8_UNLOCK()
    }
    return (jboolean) defValue;
}

template <typename F>
jobject exception_func(JNIEnv* env, jobject thiz, F&& lambda){
    JNIReturnObject out(env);
    auto valueRef = SharedWrap<JSValue>::Shared(env, thiz);
    if (valueRef && !valueRef->IsDefunct() && !valueRef->Context()->IsDefunct() &&
            !valueRef->Group()->IsDefunct()) {
        V8_ISOLATE_CTX(valueRef->Context(), isolate, context)
            Local<Value> value = valueRef->Value();

            TryCatch trycatch(isolate);
            boost::shared_ptr<JSValue> exception;

            bool failed = lambda(value, context, isolate, out);

            if (failed) {
                exception = JSValue::New(valueRef->Context(), trycatch.Exception());
                out.SetException(SharedWrap<JSValue>::New(env, exception));
            }
        V8_UNLOCK()
    }
    return out.ToJava();
}

NATIVE(JNIJSValue,jboolean,isUndefined) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsUndefined();
    }, true);
}

NATIVE(JNIJSValue,jboolean,isNull) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsNull();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isBoolean) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsBoolean();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isNumber) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsNumber();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isString) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsString();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isObject) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsObject();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isArray) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsArray();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isDate) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsDate();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isTypedArray) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        return value->IsTypedArray();
    }, false);
}

/* Comparing values */

NATIVE(JNIJSValue,jobject,isEqual) (PARAMS, jobject b)
{
    JNIReturnObject out1(env);
    auto a = SharedWrap<JSValue>::Shared(env, thiz);
    if (!a->IsDefunct()) {
        return exception_func(env, b, [a](Local<Value> b_, Local<Context> context,
                                           Isolate *isolate, JNIReturnObject out) {
            Local<Value> a_ = a->Value();
            bool result = false;
            Maybe<bool> is = a_->Equals(context, b_);
            if (!is.IsNothing()) {
                out.SetBool(is.FromMaybe(result));
            } else {
                out.SetBool(false);
            }
            return is.IsNothing();
        });
    } else {
        out1.SetBool(false);
        return out1.ToJava();
    }
}

NATIVE(JNIJSValue,jboolean,isStrictEqual) (PARAMS, jobject b)
{
    return boolean_func(env, thiz, [env, b](Local<Value> a_, Local<Context> context) {
        return boolean_func(env, b, [a_](Local<Value> b_, Local<Context> context_) {
            return a_->StrictEquals(b_);
        }, false);
    }, false);
}

/* Creating values */

NATIVE(JNIJSValue,jobject,makeUndefined) (PARAMS, jobject ctx)
{
    jobject value = nullptr;

    auto context_ = SharedWrap<JSContext>::Shared(env, ctx);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(env,
            JSValue::New(context_,Local<Value>::New(isolate,Undefined(isolate))));
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jobject,makeNull) (PARAMS, jobject ctx)
{
    jobject value = nullptr;

    auto context_ = SharedWrap<JSContext>::Shared(env, ctx);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(env,
            JSValue::New(context_,Local<Value>::New(isolate,Null(isolate))));
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jobject,makeBoolean) (PARAMS, jobject ctx, jboolean boolean)
{
    jobject value = nullptr;

    auto context_ = SharedWrap<JSContext>::Shared(env, ctx);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(env,
            JSValue::New(
                context_,
                Local<Value>::New(isolate,boolean ? v8::True(isolate):v8::False(isolate))
            ));
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jobject,makeNumber) (PARAMS, jobject ctx, jdouble number)
{
    jobject value = nullptr;

    auto context_ = SharedWrap<JSContext>::Shared(env, ctx);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(
            env,
            JSValue::New(context_,Number::New(isolate,number))
        );
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jobject,makeString) (PARAMS, jobject ctx, jstring string)
{
    jobject value = nullptr;

    auto context_ = SharedWrap<JSContext>::Shared(env, ctx);
    V8_ISOLATE_CTX(context_,isolate,context)
        const char *c_string = env->GetStringUTFChars(string, NULL);

        MaybeLocal<String> str = String::NewFromUtf8(isolate, c_string, NewStringType::kNormal);
        Local<Value> rval;
        if (str.IsEmpty()) {
            rval = Local<Value>::New(isolate,Undefined(isolate));
        } else {
            rval = str.ToLocalChecked();
        }
        env->ReleaseStringUTFChars(string, c_string);

        value = SharedWrap<JSValue>::New(
            env,
            JSValue::New(context_,rval)
        );
    V8_UNLOCK()

    return value;
}

/* Converting to and from JSON formatted strings */

NATIVE(JNIJSValue,jobject,makeFromJSONString) (PARAMS, jobject ctx, jstring string)
{
    jobject value = nullptr;

    auto context_ = SharedWrap<JSContext>::Shared(env, ctx);
    V8_ISOLATE_CTX(context_,isolate,context)
        const char *c_string = env->GetStringUTFChars(string, NULL);
        MaybeLocal<String> str = String::NewFromUtf8(isolate, c_string, NewStringType::kNormal);
        env->ReleaseStringUTFChars(string, c_string);

        if (!str.IsEmpty()) {
            MaybeLocal<Value> parsed = JSON::Parse(isolate, str.ToLocalChecked());
            if (!parsed.IsEmpty()) {
                value = SharedWrap<JSValue>::New(
                    env,
                    JSValue::New(context_,parsed.ToLocalChecked())
                );
            }
        }

        if (!value) {
            value = SharedWrap<JSValue>::New(
                env,
                JSValue::New(context_,Local<Value>::New(isolate,Undefined(isolate)))
            );
        }

    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jobject,createJSONString) (PARAMS)
{
    return exception_func(env, thiz, [env, thiz](Local<Value> inValue, Local<Context> context,
                                              Isolate *isolate, JNIReturnObject out) {
        auto valueRef = SharedWrap<JSValue>::Shared(env, thiz);
        Local<Object> json = context->Global()->Get(String::NewFromUtf8(isolate, "JSON"))->ToObject();
        Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();

        Local<Value> result = stringify->Call(json, 1, &inValue);
        out.SetReference(SharedWrap<JSValue>::New(
                env,
                JSValue::New(valueRef->Context(), result))
        );
        return false;
    });
}

/* Converting to primitive values */

NATIVE(JNIJSValue,jboolean,toBoolean) (PARAMS)
{
    return boolean_func(env, thiz, [](Local<Value> value, Local<Context> context) {
        MaybeLocal<Boolean> boolean = value->ToBoolean(context);
        return !boolean.IsEmpty() && boolean.ToLocalChecked()->Value();
    }, false);
}

NATIVE(JNIJSValue,jobject,toNumber) (PARAMS) {
    return exception_func(env, thiz, [](Local<Value> value, Local<Context> context,
                                        Isolate *isolate, JNIReturnObject out) {
        MaybeLocal<Number> number = value->ToNumber(context);
        double result = 0.0;
        if (!number.IsEmpty()) {
            out.SetNumber(number.ToLocalChecked()->Value());
        }
        return number.IsEmpty();
    });
}

NATIVE(JNIJSValue,jobject,toStringCopy) (PARAMS)
{
    return exception_func(env, thiz, [env](Local<Value> value, Local<Context> context,
                                           Isolate *isolate, JNIReturnObject out) {
        MaybeLocal<String> string = value->ToString(context);
        if (!string.IsEmpty()) {
            String::Utf8Value const str(string.ToLocalChecked());
            out.SetString(env->NewStringUTF(*str));
        } else {
            out.SetString(env->NewStringUTF(""));
        }
        return string.IsEmpty();
    });
}

NATIVE(JNIJSValue,jobject,toObject) (PARAMS)
{
    return exception_func(env, thiz, [env, thiz](Local<Value> value, Local<Context> context,
                                                 Isolate *isolate, JNIReturnObject out) {

        MaybeLocal<Object> obj = value->ToObject(context);
        if (!obj.IsEmpty()) {
            auto valueRef = SharedWrap<JSValue>::Shared(env, thiz);
            out.SetReference(SharedWrap<JSValue>::New(
                    env, JSValue::New(valueRef->Context(), value->ToObject())));
        }
        return obj.IsEmpty();
    });
}

NATIVE(JNIJSValue,void,Finalize) (PARAMS, long reference)
{
    SharedWrap<JSValue>::Dispose(reference);
}
