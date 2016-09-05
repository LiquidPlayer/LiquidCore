//
// JSValue.cpp
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

#include "JSJNI.h"

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> value = (reinterpret_cast<JSValue<Value>*>(valueRef))->Value()

NATIVE(JSValue,jboolean,isUndefined) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsUndefined();
}

NATIVE(JSValue,jboolean,isNull) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsNull();
}

NATIVE(JSValue,jboolean,isBoolean) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsBoolean();
}

NATIVE(JSValue,jboolean,isNumber) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsNumber();
}

NATIVE(JSValue,jboolean,isString) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsString();
}

NATIVE(JSValue,jboolean,isObject) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsObject();
}

NATIVE(JSValue,jboolean,isArray) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsArray();
}

NATIVE(JSValue,jboolean,isDate) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);
    return value->IsDate();
}

/* Comparing values */

NATIVE(JSValue,jobject,isEqual) (PARAMS, jlong ctxRef, jlong a, jlong b)
{
    jclass ret = env->FindClass("org/liquidplayer/v8/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    jfieldID fid = env->GetFieldID(ret ,"bool", "Z");

    bool result = false;
    JSValue<Value> *exception = nullptr;
    {
        VALUE_ISOLATE(ctxRef,a,isolate,context,a_);
        Local<Value> b_ = (reinterpret_cast<JSValue<Value>*>(b))->Value();

        TryCatch trycatch;

        Maybe<bool> is = a_->Equals(context,b_);
        if (is.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        } else {
            result = is.FromMaybe(result);
        }
    }

    env->SetBooleanField( out, fid, result);

    fid = env->GetFieldID(ret , "exception", "J");
    env->SetLongField(out, fid, reinterpret_cast<long>(exception));

    return out;
}

NATIVE(JSValue,jboolean,isStrictEqual) (PARAMS, jlong ctxRef, jlong a, jlong b)
{
    VALUE_ISOLATE(ctxRef,a,isolate,context,a_);
    Local<Value> b_ = (reinterpret_cast<JSValue<Value>*>(b))->Value();
    return a_->StrictEquals(b_);
}

/* Creating values */

NATIVE(JSValue,jlong,makeUndefined) (PARAMS, jlong ctx)
{
    V8_ISOLATE_CTX(ctx,isolate,context);

    JSValue<Value> *value =
        JSValue<Value>::New(context_,Local<Value>::New(isolate,Undefined(isolate)));
    return reinterpret_cast<long>(value);
}

NATIVE(JSValue,jlong,makeNull) (PARAMS, jlong ctx)
{
    V8_ISOLATE_CTX(ctx,isolate,context);

    JSValue<Value> *value =
        JSValue<Value>::New(context_,Local<Value>::New(isolate,Null(isolate)));
    return reinterpret_cast<long>(value);
}

NATIVE(JSValue,jlong,makeBoolean) (PARAMS, jlong ctx, jboolean boolean)
{
    V8_ISOLATE_CTX(ctx,isolate,context);

    JSValue<Value> *value =
        JSValue<Value>::New(context_,
            Local<Value>::New(isolate,boolean ? v8::True(isolate):v8::False(isolate)));
    return reinterpret_cast<long>(value);
}

NATIVE(JSValue,jlong,makeNumber) (PARAMS, jlong ctx, jdouble number)
{
    V8_ISOLATE_CTX(ctx,isolate,context);

    JSValue<Value> *value =
        JSValue<Value>::New(context_,Number::New(isolate,number));
    return reinterpret_cast<long>(value);
}

NATIVE(JSValue,jlong,makeString) (PARAMS, jlong ctx, jstring string)
{
    V8_ISOLATE_CTX(ctx,isolate,context);

    const char *c_string = env->GetStringUTFChars(string, NULL);

    MaybeLocal<String> str = String::NewFromUtf8(isolate, c_string, NewStringType::kNormal);
    Local<Value> rval;
    if (str.IsEmpty()) {
        rval = Local<Value>::New(isolate,Undefined(isolate));
    } else {
        rval = str.ToLocalChecked();
    }

    env->ReleaseStringUTFChars(string, c_string);

    JSValue<Value> *value = JSValue<Value>::New(context_,rval);
    return reinterpret_cast<long>(value);
}

/* Converting to and from JSON formatted strings */

NATIVE(JSValue,jlong,makeFromJSONString) (PARAMS, jlong ctx, jstring string)
{
    V8_ISOLATE_CTX(ctx,isolate,context);

    const char *c_string = env->GetStringUTFChars(string, NULL);
    MaybeLocal<String> str = String::NewFromUtf8(isolate, c_string, NewStringType::kNormal);
    env->ReleaseStringUTFChars(string, c_string);

    JSValue<Value> *value = nullptr;
    if (!str.IsEmpty()) {
        MaybeLocal<Value> parsed = JSON::Parse(isolate, str.ToLocalChecked());
        if (!parsed.IsEmpty())
            value = JSValue<Value>::New(context_,parsed.ToLocalChecked());
    }

    if (!value) {
        value = JSValue<Value>::New(context_,Local<Value>::New(isolate,Undefined(isolate)));
    }

    return reinterpret_cast<long>(value);
}

NATIVE(JSValue,jobject,createJSONString) (PARAMS, jlong ctxRef, jlong valueRef, jint indent)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,inValue);

    JSValue<Value> *exception = nullptr;

    jclass ret = env->FindClass("org/liquidplayer/v8/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    Local<Object> json = context->Global()->Get(String::NewFromUtf8(isolate, "JSON"))->ToObject();
    Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();

    Local<Value> result = stringify->Call(json, 1, &inValue);
    JSValue<Value> *value = JSValue<Value>::New(context_, result);

    jfieldID fid = env->GetFieldID(ret , "reference", "J");
    env->SetLongField( out, fid, reinterpret_cast<long>(value));

    fid = env->GetFieldID(ret , "exception", "J");
    env->SetLongField(out, fid, reinterpret_cast<long>(exception));

    return out;
}

/* Converting to primitive values */

NATIVE(JSValue,jboolean,toBoolean) (PARAMS, jlong ctx, jlong valueRef)
{
    VALUE_ISOLATE(ctx,valueRef,isolate,context,value);

    bool ret = false;
    MaybeLocal<Boolean> boolean = value->ToBoolean(context);
    if (!boolean.IsEmpty()) {
        ret = boolean.ToLocalChecked()->Value();
    }

    return ret;
}

NATIVE(JSValue,jobject,toNumber) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);

    jclass ret = env->FindClass("org/liquidplayer/v8/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    TryCatch trycatch;
    JSValue<Value> *exception = nullptr;

    MaybeLocal<Number> number = value->ToNumber(context);
    double result = 0.0;
    if (!number.IsEmpty()) {
        result = number.ToLocalChecked()->Value();
    } else {
        exception = JSValue<Value>::New(context_, trycatch.Exception());
    }

    jfieldID fid = env->GetFieldID(ret , "number", "D");
    env->SetDoubleField( out, fid, result);

    fid = env->GetFieldID(ret , "exception", "J");
    env->SetLongField( out, fid, reinterpret_cast<long>(exception));

    return out;
}

NATIVE(JSValue,jobject,toStringCopy) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);

    jclass ret = env->FindClass("org/liquidplayer/v8/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    TryCatch trycatch;
    JSValue<Value> *exception = nullptr;
    jstring retStr;

    MaybeLocal<String> string = value->ToString(context);
    if (!string.IsEmpty()) {
        String::Utf8Value const str(string.ToLocalChecked());
        retStr = env->NewStringUTF(*str);
    } else {
        retStr = env->NewStringUTF("");
        exception = JSValue<Value>::New(context_, trycatch.Exception());
    }

    jfieldID fid = env->GetFieldID(ret , "string", "Ljava/lang/String;");
    env->SetObjectField( out, fid, retStr);

    fid = env->GetFieldID(ret , "exception", "J");
    env->SetLongField( out, fid, reinterpret_cast<long>(exception));

    return out;
}

NATIVE(JSValue,jobject,toObject) (PARAMS, jlong ctxRef, jlong valueRef)
{
    VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value);

    jclass ret = env->FindClass("org/liquidplayer/v8/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    TryCatch trycatch;
    JSValue<Value> *exception = nullptr;

    MaybeLocal<Object> obj = value->ToObject(context);
    if (!obj.IsEmpty()) {
        jfieldID fid = env->GetFieldID(ret , "reference", "J");
        JSValue<Value> *o = JSValue<Value>::New(context_, value->ToObject());
        env->SetLongField( out, fid, reinterpret_cast<long>(o));
    } else {
        exception = JSValue<Value>::New(context_, trycatch.Exception());
    }

    jfieldID fid = env->GetFieldID(ret , "exception", "J");
    env->SetLongField( out, fid, (long) exception);

    return out;
}

/* Garbage collection */

NATIVE(JSValue,void,protect) (PARAMS, jlong ctxRef, jlong valueRef)
{
    JSValue<Value> *value = reinterpret_cast<JSValue<Value>*>(valueRef);
    value->retain();
}

NATIVE(JSValue,void,unprotect) (PARAMS, jlong ctxRef, jlong valueRef)
{
    __android_log_write(ANDROID_LOG_DEBUG, "unprotect", "Getting JSValue<Value>");
    JSValue<Value> *value = reinterpret_cast<JSValue<Value>*>(valueRef);
    char buf[128];
    sprintf(buf, "releasing value = %p", value);
    Retainer::m_debug_mutex.lock();
    bool found = (std::find(Retainer::m_debug.begin(), Retainer::m_debug.end(), value) != Retainer::m_debug.end());
    Retainer::m_debug_mutex.unlock();
    if (!found) {
        __android_log_write(ANDROID_LOG_ERROR, "unprotect", "Attempting to unprotect a dead reference!");
    }
    __android_log_write(ANDROID_LOG_DEBUG, "unprotect", buf);
    value->release();
    __android_log_write(ANDROID_LOG_DEBUG, "unprotect", "returning");
}

NATIVE(JSValue,void,setException) (PARAMS, jlong valueRef, jlong exceptionRefRef)
{
    JSValue<Value> **exception = (JSValue<Value> **)exceptionRefRef;
    *exception = reinterpret_cast<JSValue<Value> *>(valueRef);
}
