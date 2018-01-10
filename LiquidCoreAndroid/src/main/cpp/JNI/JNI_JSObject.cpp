//
// JSObject.cpp
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

#include "JNI/JNI.h"
#include "JNI/JSFunction.h"

#define VALUE_ISOLATE(valueRef,isolate,context,value) \
    auto valueRef = SharedWrap<JSValue>::Shared(env, thiz); \
    V8_ISOLATE_CTX(valueRef->Context(),isolate,context); \
    Local<Value> value = valueRef->Value();

#define V8_ISOLATE_OBJ(object,isolate,context,o) \
    VALUE_ISOLATE(object,isolate,context,__v__) \
    Local<Object> o = __v__->ToObject(context).ToLocalChecked();

NATIVE(JNIJSObject,jobject,make) (PARAMS, jobject context_)
{
    jobject value = nullptr;
    auto ctx = SharedWrap<JSContext>::Shared(env, context_);
    V8_ISOLATE_CTX(ctx,isolate,context)
        value = SharedWrap<JSValue>::New(
            env,
            JSValue::New(ctx, Object::New(isolate))
        );
    V8_UNLOCK()
    return value;
}

NATIVE(JNIJSFunction,jobject,makeFunctionWithCallback) (PARAMS, jobject jsfthis, jobject ctx, jstring name)
{
    return SharedWrap<JSValue>::New(
        env,
        JSFunction::New(env, jsfthis, ctx, name)
    );
}

NATIVE(JNIJSObject,jobject,makeArray) (PARAMS, jobject context_, jobjectArray args)
{
    JNIReturnObject ret(env);
    auto ctx = SharedWrap<JSContext>::Shared(env, context_);

    V8_ISOLATE_CTX(ctx,isolate,context)
        jsize len = env->GetArrayLength(args);

        std::shared_ptr<JSValue> exception;

        Local<Array> array = Array::New(isolate, len);

        TryCatch trycatch(isolate);

        uint32_t i;
        for (i=0; !exception && i<len; i++) {
            Local<Value> element = SharedWrap<JSValue>::Shared(
                env,
                env->GetObjectArrayElement(args, i)
            )->Value();
            if (array->Set(context, i, element).IsNothing()) {
                exception = JSValue::New(ctx, trycatch.Exception());
            }
        }

        if (!exception) {
            ret.SetReference(SharedWrap<JSValue>::New(
                env,
                JSValue::New(ctx,array)
            ));
        } else {
            ret.SetException(SharedWrap<JSValue>::New(
                env, exception
            ));
        }
    V8_UNLOCK()

    return ret.ToJava();
}

NATIVE(JNIJSObject,jobject,makeDate) (PARAMS, jobject context_, jlongArray args)
{
    auto ctx = SharedWrap<JSContext>::Shared(env, context_);
    jobject out = nullptr;

    V8_ISOLATE_CTX(ctx,isolate,context)
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

        out = SharedWrap<JSValue>::New(env, JSValue::New(ctx, date));
    V8_UNLOCK()

    return out;
}

NATIVE(JNIJSObject,jobject,makeError) (PARAMS, jobject context_, jstring message)
{
    auto ctx = SharedWrap<JSContext>::Shared(env, context_);
    jobject out = nullptr;

    V8_ISOLATE_CTX(ctx,isolate,context)
        const char *c_string = env->GetStringUTFChars(message, NULL);
        Local<String> str =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(message, c_string);

        out = SharedWrap<JSValue>::New(env, JSValue::New(ctx, Exception::Error(str)));
    V8_UNLOCK()

    return out;
}

NATIVE(JNIJSObject,jobject,makeRegExp) (PARAMS, jobject context_, jstring pattern_, jstring flags_)
{
    JNIReturnObject out(env);
    auto ctx = SharedWrap<JSContext>::Shared(env, context_);

    V8_ISOLATE_CTX(ctx,isolate,context)
        const char *c_string = env->GetStringUTFChars(pattern_, NULL);
        Local<String> pattern =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();
        env->ReleaseStringUTFChars(pattern_, c_string);

        c_string = env->GetStringUTFChars(flags_, NULL);
        RegExp::Flags flags = RegExp::Flags::kNone;
        for (size_t i=0; i<strlen(c_string); i++) {
            switch (c_string[i]) {
                case 'g': flags = (RegExp::Flags)(flags | RegExp::Flags::kGlobal);     break;
                case 'i': flags = (RegExp::Flags)(flags | RegExp::Flags::kIgnoreCase); break;
                case 'm': flags = (RegExp::Flags)(flags | RegExp::Flags::kMultiline);  break;
                default: break;
            }
        }
        env->ReleaseStringUTFChars(flags_, c_string);

        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<RegExp> regexp = RegExp::New(context, pattern, flags);
        if (regexp.IsEmpty()) {
            exception = JSValue::New(ctx, trycatch.Exception());
        }

        if (!exception) {
            out.SetReference(SharedWrap<JSValue>::New(
                env, JSValue::New(ctx, regexp.ToLocalChecked())));
        } else {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobject,makeFunction) (PARAMS, jobject context_, jstring name_,
        jstring func_, jstring sourceURL_, jint startingLineNumber)
{
    JNIReturnObject out(env);
    auto ctx = SharedWrap<JSContext>::Shared(env, context_);

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
        std::shared_ptr<JSValue> exception;

        const char *sourceURL = env->GetStringUTFChars(sourceURL_, NULL);
        ScriptOrigin script_origin(
            String::NewFromUtf8(isolate, sourceURL, NewStringType::kNormal).ToLocalChecked(),
            Integer::New(isolate, startingLineNumber)
        );
        env->ReleaseStringUTFChars(sourceURL_, sourceURL);

        MaybeLocal<Script> script = Script::Compile(context, source, &script_origin);
        if (script.IsEmpty()) {
            exception = JSValue::New(ctx, trycatch.Exception());
        }

        MaybeLocal<Value> result;

        if (!exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                exception = JSValue::New(ctx, trycatch.Exception());
            }
        }

        if (!exception) {
            Local<Function> function = Local<Function>::Cast(result.ToLocalChecked());
            function->SetName(name);
            out.SetReference(SharedWrap<JSValue>::New(env,
                JSValue::New(ctx, result.ToLocalChecked())));
        } else {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobject,getPrototype) (PARAMS)
{
    jobject out;

    V8_ISOLATE_OBJ(object,isolate,context,o)
        out = SharedWrap<JSValue>::New(env,
            JSValue::New(object->Context(), o->GetPrototype()));
    V8_UNLOCK()

    return out;
}

NATIVE(JNIJSObject,void,setPrototype) (PARAMS, jobject value)
{
    V8_ISOLATE_OBJ(object,isolate,context,o)
        o->SetPrototype(context, SharedWrap<JSValue>::Shared(env, value)->Value());
    V8_UNLOCK()
}

NATIVE(JNIJSObject,jboolean,hasProperty) (PARAMS, jstring propertyName)
{
    bool v;

    V8_ISOLATE_OBJ(object,isolate,context,o)
        const char *c_string = env->GetStringUTFChars(propertyName, NULL);
        Maybe<bool> has = o->Has(context, String::NewFromUtf8(isolate, c_string));
        env->ReleaseStringUTFChars(propertyName, c_string);

        v = has.FromMaybe(false);
    V8_UNLOCK()

    return (jboolean)v;
}

NATIVE(JNIJSObject,jobject,getProperty) (PARAMS, jstring propertyName)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
        const char *c_string = env->GetStringUTFChars(propertyName, NULL);

        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<Value> value = o->Get(context, String::NewFromUtf8(isolate, c_string));
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out.SetReference(SharedWrap<JSValue>::New(env,
                JSValue::New(object->Context(), value.ToLocalChecked())));
        } else {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
        env->ReleaseStringUTFChars(propertyName, c_string);
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobject,setProperty) (PARAMS, jstring propertyName, jobject value, jint attributes)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
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
        std::shared_ptr<JSValue> exception;

        Maybe<bool> defined = attributes ?
            o->DefineOwnProperty(
                context,
                String::NewFromUtf8(isolate, c_string),
                SharedWrap<JSValue>::Shared(env, value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, String::NewFromUtf8(isolate, c_string),
                SharedWrap<JSValue>::Shared(env, value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (exception) {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }

        env->ReleaseStringUTFChars(propertyName, c_string);
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobject,deleteProperty) (PARAMS, jstring propertyName)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
        const char *c_string = env->GetStringUTFChars(propertyName, NULL);

        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        Maybe<bool> deleted = o->Delete(context, String::NewFromUtf8(isolate, c_string));
        if (deleted.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (exception) {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }

        env->ReleaseStringUTFChars(propertyName, c_string);
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobject,getPropertyAtIndex) (PARAMS, jint propertyIndex)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<Value> value = o->Get(context, (uint32_t) propertyIndex);
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out.SetReference(SharedWrap<JSValue>::New(env,
                JSValue::New(object->Context(), value.ToLocalChecked())));
        } else {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobject,setPropertyAtIndex) (PARAMS, jint propertyIndex, jobject value)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        Maybe<bool> defined =
            o->Set(context, (uint32_t) propertyIndex,SharedWrap<JSValue>::Shared(env, value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (exception) {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jboolean,isFunction) (PARAMS) {
    bool v;

    VALUE_ISOLATE(object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return (jboolean) v;
}

NATIVE(JNIJSObject,jobject,callAsFunction) (PARAMS, jobject thisObject, jobjectArray args)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
        Local<Value> this_ = thisObject ?
            SharedWrap<JSValue>::Shared(env, thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        int i;
        jsize len = env->GetArrayLength(args);
        Local<Value> *elements = new Local<Value>[len];
        for (i=0; i<len; i++) {
            elements[i] =
                SharedWrap<JSValue>::Shared(env, env->GetObjectArrayElement(args,i))->Value();
        }

        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, len, elements);
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out.SetReference(SharedWrap<JSValue>::New(env,
                JSValue::New(object->Context(), value.ToLocalChecked())));
        } else {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }
        delete [] elements;
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jboolean,isConstructor) (PARAMS)
{
    // All functions can be constructors, yeah?  This is left over legacy from
    // JavaScriptCore.
    bool v;

    VALUE_ISOLATE(object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return (jboolean)v;
}

NATIVE(JNIJSObject,jobject,callAsConstructor) (PARAMS, jobjectArray args)
{
    JNIReturnObject out(env);

    V8_ISOLATE_OBJ(object,isolate,context,o)
        int i;
        jsize len = env->GetArrayLength(args);
        Local<Value> *elements = new Local<Value>[len];
        for (i=0; i<len; i++) {
            elements[i] =
                SharedWrap<JSValue>::Shared(env, env->GetObjectArrayElement(args,i))->Value();
        }

        TryCatch trycatch(isolate);
        std::shared_ptr<JSValue> exception;

        MaybeLocal<Value> value = o->CallAsConstructor(context, len, elements);
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out.SetReference(SharedWrap<JSValue>::New(env,
                JSValue::New(object->Context(), value.ToLocalChecked())));
        } else {
            out.SetException(SharedWrap<JSValue>::New(env, exception));
        }

        delete [] elements;
    V8_UNLOCK()

    return out.ToJava();
}

NATIVE(JNIJSObject,jobjectArray,copyPropertyNames) (PARAMS) {
    jobjectArray ret;

    V8_ISOLATE_OBJ(object,isolate,context,o)
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