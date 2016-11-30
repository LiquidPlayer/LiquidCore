//
// JSObject.cpp
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
#include "JSFunction.hpp"

#define V8_ISOLATE_OBJ(ctx,object,isolate,context,o) \
    V8_ISOLATE_CTX(ctx,isolate,context); \
    Local<Object> o = \
        reinterpret_cast<JSValue<Value>*>(object)->Value()->ToObject(context).ToLocalChecked();

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> value = (reinterpret_cast<JSValue<Value>*>(valueRef))->Value();

NATIVE(JSObject,jlong,make) (PARAMS, jlong ctx) {
    JSValue<Value> *value;
    V8_ISOLATE_CTX(ctx,isolate,context)
        value = JSValue<Value>::New(context_, Object::New(isolate));
    V8_UNLOCK()
    return reinterpret_cast<long>(value);
}

NATIVE(JSFunction,jlong,makeFunctionWithCallback) (PARAMS, jlong ctx, jstring name) {
    JSFunction<Value> *function = new JSFunction<Value>(env, thiz, ctx, name);
    return reinterpret_cast<long>(static_cast<JSValue<Value>*>(function));
}

NATIVE(JSObject,jobject,makeArray) (PARAMS, jlong ctx, jlongArray args) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_CTX(ctx,isolate,context)
        jsize len = env->GetArrayLength(args);
        jlong *values = env->GetLongArrayElements(args, 0);

        jfieldID fid = env->GetFieldID(ret , "reference", "J");

        JSValue<Value> *exception = nullptr;

        Local<Array> array = Array::New(isolate, len);

        TryCatch trycatch(isolate);

        int i;
        for (i=0; !exception && i<len; i++) {
            Local<Value> element = reinterpret_cast<JSValue<Value>*>(values[i])->Value();
            if (array->Set(context, i, element).IsNothing()) {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }
        env->ReleaseLongArrayElements(args, values, 0);

        if (!exception) {
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_,array)) );
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jlong,makeDate) (PARAMS, jlong ctx, jlongArray args) {
    jlong out;

    V8_ISOLATE_CTX(ctx,isolate,context)
        int i;
        jsize len = env->GetArrayLength(args);
        jlong *values = env->GetLongArrayElements(args, 0);

        Local<Value> date;
        if (len==0) {
            Local<Object> DATE =
                context->Global()->Get(String::NewFromUtf8(isolate, "Date"))->ToObject();
            Local<Function> now = DATE->Get(String::NewFromUtf8(isolate, "now")).As<Function>();

            date = Date::New(isolate,
                now->Call(Local<Value>::New(isolate,Null(isolate)), 0, nullptr)
                    ->ToNumber(context).ToLocalChecked()->Value());
        } else {
            date = Date::New(isolate, (double)values[0]);
        }

        env->ReleaseLongArrayElements(args, values, 0);

        out = reinterpret_cast<long>(JSValue<Value>::New(context_, date));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jlong,makeError) (PARAMS, jlong ctx, jstring message) {
    jlong out;

    V8_ISOLATE_CTX(ctx,isolate,context)
        const char *c_string = env->GetStringUTFChars(message, NULL);
        Local<String> str =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(message, c_string);

        out = reinterpret_cast<long>(JSValue<Value>::New(context_, Exception::Error(str)));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobject,makeRegExp) (PARAMS, jlong ctx, jstring pattern_, jstring flags_) {
    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_CTX(ctx,isolate,context)
        const char *c_string = env->GetStringUTFChars(pattern_, NULL);
        Local<String> pattern =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(pattern_, c_string);

        c_string = env->GetStringUTFChars(flags_, NULL);
        RegExp::Flags flags = RegExp::Flags::kNone;
        for (size_t i=0; i<strlen(c_string); i++) {
            switch (c_string[i]) {
                case 'g': flags = (RegExp::Flags) (flags | RegExp::Flags::kGlobal);     break;
                case 'i': flags = (RegExp::Flags) (flags | RegExp::Flags::kIgnoreCase); break;
                case 'm': flags = (RegExp::Flags) (flags | RegExp::Flags::kMultiline);  break;
            }
        }
        env->ReleaseStringUTFChars(flags_, c_string);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        MaybeLocal<RegExp> regexp = RegExp::New(context, pattern, flags);
        if (regexp.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "reference", "J");
        if (!exception) {
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_, regexp.ToLocalChecked())));
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobject,makeFunction) (PARAMS, jlong ctx, jstring name_,
        jstring func_, jstring sourceURL_, jint startingLineNumber) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_CTX(ctx,isolate,context)
        const char *c_string = env->GetStringUTFChars(name_, NULL);
        Local<String> name =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(name_, c_string);

        c_string = env->GetStringUTFChars(func_, NULL);
        Local<String> source =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(func_, c_string);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        const char *sourceURL = env->GetStringUTFChars(sourceURL_, NULL);
        ScriptOrigin script_origin(
            String::NewFromUtf8(isolate, sourceURL, NewStringType::kNormal).ToLocalChecked(),
            Integer::New(isolate, startingLineNumber)
        );
        env->ReleaseStringUTFChars(sourceURL_, sourceURL);

        MaybeLocal<Script> script = Script::Compile(context, source, &script_origin);
        if (script.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        MaybeLocal<Value> result;

        if (!exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!exception) {
            Local<Function> function = Local<Function>::Cast(result.ToLocalChecked());
            function->SetName(name);
            jfieldID fid = env->GetFieldID(ret , "reference", "J");
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_,
                    result.ToLocalChecked())));
        }

        jfieldID fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jlong,getPrototype) (PARAMS, jlong ctx, jlong object) {
    jlong out;

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        out = reinterpret_cast<long>(JSValue<Value>::New(context_, o->GetPrototype()));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,void,setPrototype) (PARAMS, jlong ctx, jlong object, jlong value) {
    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        o->SetPrototype(context, reinterpret_cast<JSValue<Value>*>(value)->Value());
    V8_UNLOCK()
}

NATIVE(JSObject,jboolean,hasProperty) (PARAMS, jlong ctx, jlong object, jstring propertyName) {
    bool v;

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        const char *c_string = env->GetStringUTFChars(propertyName, NULL);
        Maybe<bool> has = o->Has(context, String::NewFromUtf8(isolate, c_string));
        env->ReleaseStringUTFChars(propertyName, c_string);

        v = has.FromMaybe(false);
    V8_UNLOCK()

    return v;
}

NATIVE(JSObject,jobject,getProperty) (PARAMS, jlong ctx, jlong object,
        jstring propertyName) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        const char *c_string = env->GetStringUTFChars(propertyName, NULL);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        MaybeLocal<Value> value = o->Get(context, String::NewFromUtf8(isolate, c_string));
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "reference", "J");
        if (!exception) {
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_, value.ToLocalChecked())));
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));

        env->ReleaseStringUTFChars(propertyName, c_string);
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobject,setProperty) (PARAMS, jlong ctx, jlong object, jstring propertyName,
    jlong value, jint attributes) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        enum {
            kJSPropertyAttributeReadOnly = 1 << 1,
            kJSPropertyAttributeDontEnum = 1 << 2,
            kJSPropertyAttributeDontDelete = 1 << 3
        };

        int v8_attr = v8::None;
        if (attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if (attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if (attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        const char *c_string = env->GetStringUTFChars(propertyName, NULL);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        Maybe<bool> defined = (attributes!=0) ?
            o->DefineOwnProperty(
                context,
                String::NewFromUtf8(isolate, c_string),
                reinterpret_cast<JSValue<Value>*>(value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, String::NewFromUtf8(isolate, c_string),
                reinterpret_cast<JSValue<Value>*>(value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));

        env->ReleaseStringUTFChars(propertyName, c_string);
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobject,deleteProperty) (PARAMS, jlong ctx, jlong object, jstring propertyName) {
    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        const char *c_string = env->GetStringUTFChars(propertyName, NULL);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        Maybe<bool> deleted = o->Delete(context, String::NewFromUtf8(isolate, c_string));
        if (deleted.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));

        env->ReleaseStringUTFChars(propertyName, c_string);
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobject,getPropertyAtIndex) (PARAMS, jlong ctx, jlong object,
    jint propertyIndex) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        MaybeLocal<Value> value = o->Get(context, propertyIndex);
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "reference", "J");
        if (!exception) {
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_, value.ToLocalChecked())));
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobject,setPropertyAtIndex) (PARAMS, jlong ctx, jlong object,
    jint propertyIndex, jlong value) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        Maybe<bool> defined =
            o->Set(context, propertyIndex,
                reinterpret_cast<JSValue<Value>*>(value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jboolean,isFunction) (PARAMS, jlong ctx, jlong object) {
    bool v;

    VALUE_ISOLATE(ctx,object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return v;
}

NATIVE(JSObject,jobject,callAsFunction) (PARAMS, jlong ctx, jlong object,
    jlong thisObject, jlongArray args) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        Local<Value> this_ = thisObject ?
            reinterpret_cast<JSValue<Value>*>(thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        int i;
        jsize len = env->GetArrayLength(args);
        jlong *values = env->GetLongArrayElements(args, 0);
        Local<Value> *elements = new Local<Value>[len];
        for (i=0; i<len; i++) {
            elements[i] = reinterpret_cast<JSValue<Value>*>(values[i])->Value();
        }
        env->ReleaseLongArrayElements(args, values, 0);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, len, elements);
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "reference", "J");
        if (!exception) {
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_, value.ToLocalChecked())));
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));

        delete [] elements;
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jboolean,isConstructor) (PARAMS, jlong ctx, jlong object) {
    // All functions can be constructors, yeah?  This is left over legacy from
    // JavaScriptCore.
    bool v;

    VALUE_ISOLATE(ctx,object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return v;
}

NATIVE(JSObject,jobject,callAsConstructor) (PARAMS, jlong ctx, jlong object,
    jlongArray args) {

    jclass ret = env->FindClass("org/liquidplayer/javascript/JSValue$JNIReturnObject");
    jmethodID cid = env->GetMethodID(ret,"<init>","()V");
    jobject out = env->NewObject(ret, cid);

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        int i;
        jsize len = env->GetArrayLength(args);
        jlong *values = env->GetLongArrayElements(args, 0);
        Local<Value> *elements = new Local<Value>[len];
        for (i=0; i<len; i++) {
            elements[i] = reinterpret_cast<JSValue<Value>*>(values[i])->Value();
        }
        env->ReleaseLongArrayElements(args, values, 0);

        TryCatch trycatch(isolate);
        JSValue<Value> *exception = nullptr;

        MaybeLocal<Value> value = o->CallAsConstructor(context, len, elements);
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        jfieldID fid = env->GetFieldID(ret , "reference", "J");
        if (!exception) {
            env->SetLongField( out, fid,
                reinterpret_cast<long>(JSValue<Value>::New(context_, value.ToLocalChecked())));
        }

        fid = env->GetFieldID(ret , "exception", "J");
        env->SetLongField( out, fid, reinterpret_cast<long>(exception));

        delete [] elements;
    V8_UNLOCK()

    return out;
}

NATIVE(JSObject,jobjectArray,copyPropertyNames) (PARAMS, jlong ctx, jlong object) {
    jobjectArray ret;

    V8_ISOLATE_OBJ(ctx,object,isolate,context,o)
        Local<Array> names = o->GetPropertyNames(context).ToLocalChecked();

        ret = (jobjectArray) env->NewObjectArray(
            names->Length(),
            env->FindClass("java/lang/String"),
            env->NewStringUTF(""));
        for (size_t i=0; i<names->Length(); i++) {
            Local<String> property =
                names->Get(context, i).ToLocalChecked()->ToString(context).ToLocalChecked();
            String::Utf8Value const str(property);
            env->SetObjectArrayElement(ret, i, env->NewStringUTF(*str));
        }
    V8_UNLOCK()

    return ret;
}