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

#define VALUE_ISOLATE(valueRef,isolate,context,value) \
    auto valueRef = SharedWrap<JSValue>::Shared(env, thiz); \
    V8_ISOLATE_CTX(valueRef->Context(),isolate,context); \
    Local<Value> value = valueRef->Value();

NATIVE(JNIJSValue,jboolean,isUndefined) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsUndefined();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isNull) (PARAMS)
{

    bool v;
    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsNull();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isBoolean) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsBoolean();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isNumber) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsNumber();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isString) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsString();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isObject) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsObject();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isArray) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsArray();
    V8_UNLOCK()

    return v;
}

NATIVE(JNIJSValue,jboolean,isDate) (PARAMS)
{
    bool v;

    VALUE_ISOLATE(valueRef,isolate,context,value)
        v = value->IsDate();
    V8_UNLOCK()

    return v;
}

/* Comparing values */

NATIVE(JNIJSValue,jobject,isEqual) (PARAMS, jobject b)
{
    JNIReturnObject out(env);

    bool result = false;
    std::shared_ptr<JSValue> exception;
    {
        VALUE_ISOLATE(a,isolate,context,a_)
            Local<Value> b_ = SharedWrap<JSValue>::Shared(env, b)->Value();

            TryCatch trycatch(isolate);

            Maybe<bool> is = a_->Equals(context,b_);
            if (is.IsNothing()) {
                exception = JSValue::New(a->Context(), trycatch.Exception());
            } else {
                result = is.FromMaybe(result);
            }
        V8_UNLOCK()
    }
    out.SetBool(result);

    if (exception) {
        out.SetException(SharedWrap<JSValue>::New(env, exception));
    }

    return out.ToJava();
}

NATIVE(JNIJSValue,jboolean,isStrictEqual) (PARAMS, jobject b)
{
    bool v;
    VALUE_ISOLATE(a,isolate,context,a_)
        Local<Value> b_ = SharedWrap<JSValue>::Shared(env, b)->Value();
        v = a_->StrictEquals(b_);
    V8_UNLOCK()
    return v;
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

NATIVE(JNIJSValue,jobject,createJSONString) (PARAMS, jint indent)
{
    JNIReturnObject ret(env);

    VALUE_ISOLATE(valueRef,isolate,context,inValue)
        Local<Object> json = context->Global()->Get(String::NewFromUtf8(isolate, "JSON"))->ToObject();
        Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();

        Local<Value> result = stringify->Call(json, 1, &inValue);
        ret.SetReference(SharedWrap<JSValue>::New(
            env,
            JSValue::New(valueRef->Context(), result))
        );
    V8_UNLOCK()

    return ret.ToJava();
}

/* Converting to primitive values */

NATIVE(JNIJSValue,jboolean,toBoolean) (PARAMS)
{
    bool ret = false;
    VALUE_ISOLATE(valueRef,isolate,context,value)
        MaybeLocal<Boolean> boolean = value->ToBoolean(context);
        if (!boolean.IsEmpty()) {
            ret = boolean.ToLocalChecked()->Value();
        }
    V8_UNLOCK()
    return ret;
}

NATIVE(JNIJSValue,jobject,toNumber) (PARAMS)
{
    JNIReturnObject out(env);

    VALUE_ISOLATE(valueRef,isolate,context,value)
        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<Number> number = value->ToNumber(context);
        double result = 0.0;
        if (!number.IsEmpty()) {
            result = number.ToLocalChecked()->Value();
        } else {
            exception = JSValue::New(valueRef->Context(), trycatch.Exception());
        }

        out.SetNumber(result);
        if (exception) {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
    V8_UNLOCK()
    return out.ToJava();
}

NATIVE(JNIJSValue,jobject,toStringCopy) (PARAMS)
{
    JNIReturnObject out(env);

    VALUE_ISOLATE(valueRef,isolate,context,value)
        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;
        jstring retStr;

        MaybeLocal<String> string = value->ToString(context);
        if (!string.IsEmpty()) {
            String::Utf8Value const str(string.ToLocalChecked());
            retStr = env->NewStringUTF(*str);
        } else {
            retStr = env->NewStringUTF("");
            exception = JSValue::New(valueRef->Context(), trycatch.Exception());
        }

        out.SetString(retStr);

        if (exception) {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
    V8_UNLOCK()
    return out.ToJava();
}

NATIVE(JNIJSValue,jobject,toObject) (PARAMS)
{
    JNIReturnObject out(env);

    VALUE_ISOLATE(valueRef,isolate,context,value)
        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<Object> obj = value->ToObject(context);
        if (!obj.IsEmpty()) {
            out.SetReference(
                SharedWrap<JSValue>::New(
                    env,
                    JSValue::New(valueRef->Context(), value->ToObject())
                )
            );
        } else {
            out.SetException(
                SharedWrap<JSValue>::New(
                    env,
                    JSValue::New(valueRef->Context(), trycatch.Exception())
                )
            );
        }
    V8_UNLOCK()
    return out.ToJava();
}

NATIVE(JNIJSValue,void,Finalize) (PARAMS, long reference)
{
    SharedWrap<JSValue>::Dispose(reference);
}
