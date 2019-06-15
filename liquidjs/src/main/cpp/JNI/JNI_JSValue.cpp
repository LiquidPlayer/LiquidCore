/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */

#include "JNI/JNI.h"
#include "JNI/JNIJSException.h"

#define IS_FUNCTION(TYPE) \
NATIVE(JNIJSValue,jboolean,is##TYPE) (STATIC, jlong thiz) {\
    auto valueRef = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), thiz); \
    bool defValue = false; \
    if (valueRef && !valueRef->IsDefunct() && !valueRef->Context()->IsDefunct() && \
        !valueRef->Group()->IsDefunct()) { \
        V8_ISOLATE_CTX(valueRef->Context(), isolate, context) \
            Local<Value> value = valueRef->Value(); \
            defValue = value->Is##TYPE(); \
        V8_UNLOCK() \
    } \
    return (jboolean) defValue; \
}

IS_FUNCTION(Undefined)
IS_FUNCTION(Null)
IS_FUNCTION(Boolean)
IS_FUNCTION(Number)
IS_FUNCTION(String)
IS_FUNCTION(Array)
IS_FUNCTION(Date)
IS_FUNCTION(TypedArray)
IS_FUNCTION(Int8Array)
IS_FUNCTION(Int16Array)
IS_FUNCTION(Int32Array)
IS_FUNCTION(Uint8Array)
IS_FUNCTION(Uint16Array)
IS_FUNCTION(Uint32Array)
IS_FUNCTION(Uint8ClampedArray)
IS_FUNCTION(Float32Array)
IS_FUNCTION(Float64Array)

/* Comparing values */

NATIVE(JNIJSValue,jboolean,isEqual) (STATIC, jlong a_, jlong b_)
{
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


NATIVE(JNIJSValue,jboolean,isStrictEqual) (STATIC, jlong valueRef, jlong b)
{
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

NATIVE(JNIJSValue,jlong,makeNumber) (STATIC, jlong ctxRef, jdouble number)
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

NATIVE(JNIJSValue,jlong,makeString) (STATIC, jlong ctxRef, jstring string)
{
    jlong value = 0;

    auto context_ = SharedWrap<JSContext>::Shared(ctxRef);
    const char *c_string = env->GetStringUTFChars(string, nullptr);
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

NATIVE(JNIJSValue,jlong,makeFromJSONString) (STATIC, jlong ctxRef, jstring string)
{
    jlong value = 0;
    const char *c_string = env->GetStringUTFChars(string, nullptr);

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

NATIVE(JNIJSValue,jlong,createJSONString) (STATIC, jlong valueRef)
{
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    jlong out = 0;

    if (!value->IsDefunct()) {
        V8_ISOLATE_CTX(value->Context(), isolate, context)
            Local<Value> inValue = value->Value();
            Local<Object> json = context->Global()->Get(
                    String::NewFromUtf8(value->isolate(), "JSON"))->ToObject(context).ToLocalChecked();
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

NATIVE(JNIJSValue,jboolean,toBoolean) (STATIC, jlong thiz) {
    auto valueRef = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), thiz);
    bool defValue = false;
    if (valueRef && !valueRef->IsDefunct() && !valueRef->Context()->IsDefunct() &&
        !valueRef->Group()->IsDefunct()) {
        V8_ISOLATE_CTX(valueRef->Context(), isolate, context)
            Local<Value> value = valueRef->Value();
            MaybeLocal<Boolean> boolean = value->ToBoolean(context);
            defValue = !boolean.IsEmpty() && boolean.ToLocalChecked()->Value();
        V8_UNLOCK()
    }
    return (jboolean) defValue;
}


NATIVE(JNIJSValue,jdouble,toNumber) (STATIC, jlong valueRef) {
    double out = 0.0;
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

NATIVE(JNIJSValue,jstring,toStringCopy) (STATIC, jlong valueRef) {
    jstring out = nullptr;
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    boost::shared_ptr<JSValue> exception;
    const char *s = nullptr;

    V8_ISOLATE_CTX(value->Context(), isolate, context)

        TryCatch trycatch(isolate);

        MaybeLocal<String> string = value->Value()->ToString(context);
        if (!string.IsEmpty()) {
            String::Utf8Value const str(isolate, string.ToLocalChecked());
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

NATIVE(JNIJSValue,jlong,toObject) (STATIC, jlong valueRef) {
    jlong out = 0;
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE_CTX(value->Context(), isolate, context)

        TryCatch trycatch(isolate);

        MaybeLocal<Object> obj = value->Value()->ToObject(context);
        if (!obj.IsEmpty()) {
            auto v = SharedWrap<JSValue>::Shared(value->Context(), valueRef);
            out = SharedWrap<JSValue>::New(
                    JSValue::New(v->Context(), value->Value()->ToObject(context).ToLocalChecked()));
        } else {
            exception = JSValue::New(value->Context(), trycatch.Exception());
        }
    V8_UNLOCK()
    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
    return out;
}

NATIVE(JNIJSValue,jlong,canonicalReference) (STATIC, jlong valueRef) {
    return SharedWrap<JSValue>::CanonicalReference(valueRef);
}

NATIVE(JNIJSValue,void,Finalize) (STATIC, jlong reference)
{
    SharedWrap<JSValue>::Dispose(reference);
}
