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
#include "JNI/JNIJSException.h"

template <typename F>
jboolean boolean_func(jlong thiz, F&& lambda, bool defValue){
    if (!ISPOINTER(thiz)) {
        __android_log_assert("!ISPOINTER", "boolean_func", "SharedWrap<JSValue> is not a pointer");
    }
    auto valueRef = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), thiz);
    if (valueRef && !valueRef->IsDefunct() && !valueRef->Context()->IsDefunct() &&
            !valueRef->Group()->IsDefunct()) {
        V8_ISOLATE_CTX(valueRef->Context(), isolate, context)
            Local<Value> value = valueRef->Value();
            defValue = lambda(value, context);
        V8_UNLOCK()
    }
    return (jboolean) defValue;
}

NATIVE(JNIJSValue,jboolean,isUndefined) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsUndefined();
    }, true);
}

NATIVE(JNIJSValue,jboolean,isNull) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsNull();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isBoolean) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsBoolean();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isNumber) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsNumber();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isString) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsString();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isObject) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsObject();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isArray) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsArray();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isDate) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsDate();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isTypedArray) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsTypedArray();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isInt8Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsInt8Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isInt16Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsInt16Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isInt32Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsInt32Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isUint8Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsUint8Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isUint8ClampedArray) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsUint8ClampedArray();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isUint16Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsUint16Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isUint32Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsUint32Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isFloat32Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsFloat32Array();
    }, false);
}

NATIVE(JNIJSValue,jboolean,isFloat64Array) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        return value->IsFloat64Array();
    }, false);
}

/* Comparing values */

NATIVE(JNIJSValue,jboolean,isEqual) (PARAMS, jlong a_, jlong b_)
{
    if (!ISPOINTER(a_)) {
        __android_log_assert("!ISPOINTER(a_)", "JNIJSValue.isEqual", "a_ must be pointer");
    }
    auto a = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), a_);
    boost::shared_ptr<JSValue> exception;
    bool out = false;
    if (!a->IsDefunct()) {
        V8_ISOLATE_CTX(a->Context(), isolate, context)
            auto b = SharedWrap<JSValue>::Shared(a->Context(), b_);

            TryCatch trycatch(isolate);

            bool result = false;
            Maybe<bool> is = a->Value()->Equals(context, b->Value());
            if (!is.IsNothing()) {
                out = is.FromMaybe(result);
            } else {
                exception = JSValue::New(a->Context(), trycatch.Exception());
            }
        V8_UNLOCK()

        if (exception) {
            JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
        }
    }
    return (jboolean) out;
}


NATIVE(JNIJSValue,jboolean,isStrictEqual) (PARAMS, jlong valueRef, jlong b)
{
    if (!ISPOINTER(valueRef)) {
        __android_log_assert("!ISPOINTER(valueRef)", "JNIJSValue.isStrictEqual", "valueRef must be pointer");
    }
    bool ret = false;
    auto a = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    if (a && !a->IsDefunct() && !a->Context()->IsDefunct() && !a->Group()->IsDefunct()) {
        V8_ISOLATE_CTX(a->Context(), isolate, context)
            auto b_ = SharedWrap<JSValue>::Shared(a->Context(), b);
            ret = a->Value()->StrictEquals(b_->Value());
        V8_UNLOCK()
    }

    return (jboolean) ret;
}

/* Creating values */

NATIVE(JNIJSValue,jlong,makeUndefined) (PARAMS, jlong ctxRef)
{
    jlong value;

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(
                JSValue::New(context_,Local<Value>::New(isolate,Undefined(isolate))));
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jlong,makeNull) (PARAMS, jlong ctxRef)
{
    jlong value = 0;

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(
            JSValue::New(context_,Local<Value>::New(isolate,Null(isolate))));
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jlong,makeBoolean) (PARAMS, jlong ctxRef, jboolean boolean)
{
    jlong value = 0;

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(
            JSValue::New(
                context_,
                Local<Value>::New(isolate,boolean ? v8::True(isolate):v8::False(isolate))
            ));
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jlong,makeNumber) (PARAMS, jlong ctxRef, jdouble number)
{
    jlong value = 0;

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    V8_ISOLATE_CTX(context_,isolate,context)
        value = SharedWrap<JSValue>::New(
            JSValue::New(context_,Number::New(isolate,number))
        );
    V8_UNLOCK()

    return value;
}

NATIVE(JNIJSValue,jlong,makeString) (PARAMS, jlong ctxRef, jstring string)
{
    jlong value = 0;

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    const char *c_string = env->GetStringUTFChars(string, NULL);
    V8_ISOLATE_CTX(context_,isolate,context)

        MaybeLocal<String> str = String::NewFromUtf8(isolate, c_string, NewStringType::kNormal);
        Local<Value> rval;
        if (str.IsEmpty()) {
            rval = Local<Value>::New(isolate,Undefined(isolate));
        } else {
            rval = str.ToLocalChecked();
        }

        value = SharedWrap<JSValue>::New(
            JSValue::New(context_,rval)
        );
    V8_UNLOCK()
    env->ReleaseStringUTFChars(string, c_string);

    return value;
}

/* Converting to and from JSON formatted strings */

NATIVE(JNIJSValue,jlong,makeFromJSONString) (PARAMS, jlong ctxRef, jstring string)
{
    jlong value = 0;
    const char *c_string = env->GetStringUTFChars(string, NULL);

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    V8_ISOLATE_CTX(context_,isolate,context)
        MaybeLocal<String> str = String::NewFromUtf8(isolate, c_string, NewStringType::kNormal);

        if (!str.IsEmpty()) {
            MaybeLocal<Value> parsed = JSON::Parse(isolate, str.ToLocalChecked());
            if (!parsed.IsEmpty()) {
                value = SharedWrap<JSValue>::New(
                    JSValue::New(context_,parsed.ToLocalChecked())
                );
            }
        }

        if (!value) {
            value = SharedWrap<JSValue>::New(
                JSValue::New(context_,Local<Value>::New(isolate,Undefined(isolate)))
            );
        }

    V8_UNLOCK()

    env->ReleaseStringUTFChars(string, c_string);
    return value;
}

NATIVE(JNIJSValue,jlong,createJSONString) (PARAMS, jlong valueRef)
{
    if (!ISPOINTER(valueRef)) {
        __android_log_assert("!ISPOINTER(a_)", "JNIJSValue.isEqual", "valueRef must be pointer");
    }
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    jlong out = 0;

    if (!value->IsDefunct()) {
        V8_ISOLATE_CTX(value->Context(), isolate, context)
            Local<Value> inValue = value->Value();
            Local<Object> json = context->Global()->Get(
                    String::NewFromUtf8(value->isolate(), "JSON"))->ToObject();
            Local<Function> stringify = json->Get(
                    String::NewFromUtf8(value->isolate(), "stringify")).As<Function>();
            Local<Value> result = stringify->Call(json, 1, &inValue);
            out = SharedWrap<JSValue>::New(
                    JSValue::New(value->Context(), result));
        V8_UNLOCK()
    }

    return out;
}

/* Converting to primitive values */

NATIVE(JNIJSValue,jboolean,toBoolean) (PARAMS, jlong valueRef)
{
    return boolean_func(valueRef, [](Local<Value> value, Local<Context> context) {
        MaybeLocal<Boolean> boolean = value->ToBoolean(context);
        return !boolean.IsEmpty() && boolean.ToLocalChecked()->Value();
    }, false);
}

NATIVE(JNIJSValue,jdouble,toNumber) (PARAMS, jlong valueRef) {
    double out = 0.0;
    if (!ISPOINTER(valueRef)) {
        __android_log_assert("!ISPOINTER", "toNumber", "SharedWrap<JSValue> is not a pointer");
    }
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE_CTX(value->Context(), isolate, context)

        TryCatch trycatch(isolate);

        MaybeLocal<Number> number = value->Value()->ToNumber(context);
        if (!number.IsEmpty()) {
            out = number.ToLocalChecked()->Value();
        } else {
            exception = JSValue::New(value->Context(), trycatch.Exception());
        }
    V8_UNLOCK()
    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
    return out;
}

NATIVE(JNIJSValue,jstring,toStringCopy) (PARAMS, jlong valueRef) {
    jstring out = nullptr;
    if (!ISPOINTER(valueRef)) {
        __android_log_assert("!ISPOINTER", "toStringCopy", "SharedWrap<JSValue> is not a pointer");
    }
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    boost::shared_ptr<JSValue> exception;
    const char *s = nullptr;

    V8_ISOLATE_CTX(value->Context(), isolate, context)

        TryCatch trycatch(isolate);

        MaybeLocal<String> string = value->Value()->ToString(context);
        if (!string.IsEmpty()) {
            String::Utf8Value const str(string.ToLocalChecked());
            s = strdup(*str);
        } else {
            s = strdup("");
            exception = JSValue::New(value->Context(), trycatch.Exception());
        }
    V8_UNLOCK()

    out = env->NewStringUTF(s);
    free((void*)s);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSValue,jlong,toObject) (PARAMS, jlong valueRef) {
    jlong out = 0;
    if (!ISPOINTER(valueRef)) {
        __android_log_assert("!ISPOINTER", "toObject", "SharedWrap<JSValue> is not a pointer");
    }
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE_CTX(value->Context(), isolate, context)

        TryCatch trycatch(isolate);

        MaybeLocal<Object> obj = value->Value()->ToObject(context);
        if (!obj.IsEmpty()) {
            auto v = SharedWrap<JSValue>::Shared(value->Context(), valueRef);
            out = SharedWrap<JSValue>::New(
                    JSValue::New(v->Context(), value->Value()->ToObject()));
        } else {
            exception = JSValue::New(value->Context(), trycatch.Exception());
        }
    V8_UNLOCK()
    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
    return out;
}

NATIVE(JNIJSValue,void,Finalize) (PARAMS, jlong reference)
{
    if (!ISPOINTER(reference)) {
        __android_log_assert("!ISPOINTER", "Finalize", "reference is not a pointer");
    }
    SharedWrap<JSValue>::Dispose(reference);
}
