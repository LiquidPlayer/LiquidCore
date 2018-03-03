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
#include "JNI/JNIJSException.h"

#define VALUE_ISOLATE(objRef,valueRef,isolate,context,value) \
    auto valueRef = SharedWrap<JSValue>::Shared(objRef); \
    V8_ISOLATE_CTX(valueRef->Context(),isolate,context); \
    Local<Value> value = valueRef->Value();

#define V8_ISOLATE_OBJ(objRef,object,isolate,context,o) \
    VALUE_ISOLATE(objRef,object,isolate,context,__v__) \
    Local<Object> o = __v__->ToObject(context).ToLocalChecked();

NATIVE(JNIJSObject,jlong,make) (PARAMS, jlong context_)
{
    jlong value = 0;
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    V8_ISOLATE_CTX(ctx,isolate,context)
        value = SharedWrap<JSValue>::New(
            JSValue::New(ctx, Object::New(isolate))
        );
    V8_UNLOCK()
    return value;
}

NATIVE(JNIJSFunction,jlong,makeFunctionWithCallback) (PARAMS, jobject jsfthis, jlong ctx, jstring name)
{
    return SharedWrap<JSValue>::New(JSFunction::New(env, jsfthis, ctx, name));
}

NATIVE(JNIJSFunction,void,setException) (PARAMS, jlong funcRef, jlong valueRef)
{
    auto func = SharedWrap<JSValue>::Shared(funcRef);
    JSFunction *jsfunc = static_cast<JSFunction*>(&* func);
    jsfunc->setException(SharedWrap<JSValue>::Shared(valueRef));
}

NATIVE(JNIJSObject,jlong,makeArray) (PARAMS, jlong context_, jlongArray args)
{
    jlong ret = 0;
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    boost::shared_ptr<JSValue> exception;
    jsize len = env->GetArrayLength(args);
    jlong *args_ = env->GetLongArrayElements(args,nullptr);

    V8_ISOLATE_CTX(ctx,isolate,context)

        Local<Array> array = Array::New(isolate, len);

        TryCatch trycatch(isolate);

        uint32_t i;
        for (i=0; !exception && (jsize)i<len; i++) {
            Local<Value> element = SharedWrap<JSValue>::Shared(args_[i])->Value();
            if (array->Set(context, i, element).IsNothing()) {
                exception = JSValue::New(ctx, trycatch.Exception());
            }
        }

        if (!exception) {
            ret = SharedWrap<JSValue>::New(JSValue::New(ctx,array));
        }
    V8_UNLOCK()

    env->ReleaseLongArrayElements(args, args_, 0);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return ret;
}

NATIVE(JNIJSObject,jlong,makeDate) (PARAMS, jlong context_, jlongArray args)
{
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    jlong out = 0;
    jsize len = env->GetArrayLength(args);
    jlong *values = env->GetLongArrayElements(args, 0);

    V8_ISOLATE_CTX(ctx,isolate,context)

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

        out = SharedWrap<JSValue>::New(JSValue::New(ctx, date));
    V8_UNLOCK()

    env->ReleaseLongArrayElements(args, values, 0);

    return out;
}

NATIVE(JNIJSObject,jlong,makeError) (PARAMS, jlong context_, jstring message)
{
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    jlong out = 0;
    const char *c_string = env->GetStringUTFChars(message, NULL);

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<String> str =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();

        out = SharedWrap<JSValue>::New(JSValue::New(ctx, Exception::Error(str)));
    V8_UNLOCK()

    env->ReleaseStringUTFChars(message, c_string);
    return out;
}

NATIVE(JNIJSObject,jlong,makeRegExp) (PARAMS, jlong context_, jstring pattern_, jstring flags_)
{
    jlong out = 0;
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    boost::shared_ptr<JSValue> exception;
    const char *_pattern = env->GetStringUTFChars(pattern_, NULL);
    const char *_flags = env->GetStringUTFChars(flags_, NULL);

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<String> pattern =
            String::NewFromUtf8(isolate, _pattern, NewStringType::kNormal).ToLocalChecked();

        RegExp::Flags flags = RegExp::Flags::kNone;
        for (size_t i=0; i<strlen(_flags); i++) {
            switch (_flags[i]) {
                case 'g': flags = (RegExp::Flags)(flags | RegExp::Flags::kGlobal);     break;
                case 'i': flags = (RegExp::Flags)(flags | RegExp::Flags::kIgnoreCase); break;
                case 'm': flags = (RegExp::Flags)(flags | RegExp::Flags::kMultiline);  break;
                default: break;
            }
        }

        TryCatch trycatch(isolate);

        MaybeLocal<RegExp> regexp = RegExp::New(context, pattern, flags);
        if (regexp.IsEmpty()) {
            exception = JSValue::New(ctx, trycatch.Exception());
        }

        if (!exception) {
            out = SharedWrap<JSValue>::New(JSValue::New(ctx, regexp.ToLocalChecked()));
        }
    V8_UNLOCK()

    env->ReleaseStringUTFChars(pattern_, _pattern);
    env->ReleaseStringUTFChars(flags_, _flags);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
    return out;
}

NATIVE(JNIJSObject,jlong,makeFunction) (PARAMS, jlong context_, jstring name_,
        jstring func_, jstring sourceURL_, jint startingLineNumber)
{
    jlong out = 0;
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    boost::shared_ptr<JSValue> exception;
    const char *_name = env->GetStringUTFChars(name_, NULL);
    const char *_func = env->GetStringUTFChars(func_, NULL);
    const char *_sourceURL = env->GetStringUTFChars(sourceURL_, NULL);

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<String> name =
            String::NewFromUtf8(isolate, _name, NewStringType::kNormal).ToLocalChecked();

        Local<String> source =
            String::NewFromUtf8(isolate, _func, NewStringType::kNormal).ToLocalChecked();

        TryCatch trycatch(isolate);

        ScriptOrigin script_origin(
            String::NewFromUtf8(isolate, _sourceURL, NewStringType::kNormal).ToLocalChecked(),
            Integer::New(isolate, startingLineNumber)
        );

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
            out = SharedWrap<JSValue>::New(JSValue::New(ctx, result.ToLocalChecked()));
        }
    V8_UNLOCK()

    env->ReleaseStringUTFChars(name_, _name);
    env->ReleaseStringUTFChars(func_, _func);
    env->ReleaseStringUTFChars(sourceURL_, _sourceURL);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSObject,jlong,getPrototype) (PARAMS, jlong objRef)
{
    jlong out;

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        out = SharedWrap<JSValue>::New(JSValue::New(object->Context(), o->GetPrototype()));
    V8_UNLOCK()

    return out;
}

NATIVE(JNIJSObject,void,setPrototype) (PARAMS, jlong objRef, jlong valueRef)
{
    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        o->SetPrototype(context, SharedWrap<JSValue>::Shared(valueRef)->Value());
    V8_UNLOCK()
}

NATIVE(JNIJSObject,jboolean,hasProperty) (PARAMS, jlong objRef, jstring propertyName)
{
    bool v;
    const char *c_string = env->GetStringUTFChars(propertyName, NULL);

    V8_ISOLATE_OBJ(objRef, object,isolate,context,o)
        Maybe<bool> has = o->HasOwnProperty(context, String::NewFromUtf8(isolate, c_string));

        v = has.FromMaybe(false);
    V8_UNLOCK()

    env->ReleaseStringUTFChars(propertyName, c_string);
    return (jboolean)v;
}

NATIVE(JNIJSObject,jlong,getProperty) (PARAMS, jlong objRef, jstring propertyName)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;
    const char *c_string = env->GetStringUTFChars(propertyName, NULL);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, String::NewFromUtf8(isolate, c_string));
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out = SharedWrap<JSValue>::New(JSValue::New(object->Context(), value.ToLocalChecked()));
        }
    V8_UNLOCK()

    env->ReleaseStringUTFChars(propertyName, c_string);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSObject,void,setProperty) (PARAMS, jlong objRef, jstring propertyName, jlong value, jint attributes)
{
    boost::shared_ptr<JSValue> exception;
    const char *c_string = env->GetStringUTFChars(propertyName, NULL);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        enum {
            kJSPropertyAttributeReadOnly = 1 << 1,
            kJSPropertyAttributeDontEnum = 1 << 2,
            kJSPropertyAttributeDontDelete = 1 << 3
        };

        int v8_attr = v8::None;
        if (attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if (attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if (attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        TryCatch trycatch(isolate);

        Maybe<bool> defined = attributes ?
            o->DefineOwnProperty(
                context,
                String::NewFromUtf8(isolate, c_string),
                SharedWrap<JSValue>::Shared(value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, String::NewFromUtf8(isolate, c_string),
                SharedWrap<JSValue>::Shared(value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }
    V8_UNLOCK()

    env->ReleaseStringUTFChars(propertyName, c_string);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
}

NATIVE(JNIJSObject,jboolean,deleteProperty) (PARAMS, jlong objRef, jstring propertyName)
{
    jboolean out = (jboolean) false;
    boost::shared_ptr<JSValue> exception;
    const char *c_string = env->GetStringUTFChars(propertyName, NULL);

    V8_ISOLATE_OBJ(objRef, object,isolate,context,o)
        TryCatch trycatch(isolate);

        Maybe<bool> deleted = o->Delete(context, String::NewFromUtf8(isolate, c_string));
        if (deleted.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        } else {
            out = (jboolean) deleted.FromJust();
        }
    V8_UNLOCK()

    env->ReleaseStringUTFChars(propertyName, c_string);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSObject,jlong,getPropertyAtIndex) (PARAMS, jlong objRef, jint propertyIndex)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE_OBJ(objRef, object,isolate,context,o)
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, (uint32_t) propertyIndex);
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out = SharedWrap<JSValue>::New(JSValue::New(object->Context(), value.ToLocalChecked()));
        }
    V8_UNLOCK()

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSObject,void,setPropertyAtIndex) (PARAMS, jlong objRef, jint propertyIndex, jlong value)
{
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        TryCatch trycatch(isolate);

        Maybe<bool> defined =
            o->Set(context, (uint32_t) propertyIndex,SharedWrap<JSValue>::Shared(value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

    V8_UNLOCK()

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
}

NATIVE(JNIJSObject,jboolean,isFunction) (PARAMS, jlong objRef)
{
    bool v;

    VALUE_ISOLATE(objRef,object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return (jboolean) v;
}

NATIVE(JNIJSObject,jlong,callAsFunction) (PARAMS, jlong objRef, jlong thisObject, jlongArray args)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;
    jsize len = env->GetArrayLength(args);
    jlong *args_ = env->GetLongArrayElements(args,nullptr);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        Local<Value> this_ = thisObject ?
            SharedWrap<JSValue>::Shared(thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        int i;
        Local<Value> elements[len];
        for (i=0; i<len; i++) {
            elements[i] = SharedWrap<JSValue>::Shared(args_[i])->Value();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, len, elements);
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out = SharedWrap<JSValue>::New(JSValue::New(object->Context(), value.ToLocalChecked()));
        }
    V8_UNLOCK()

    env->ReleaseLongArrayElements(args, args_, 0);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSObject,jboolean,isConstructor) (PARAMS, jlong objRef)
{
    // All functions can be constructors, yeah?  This is left over legacy from
    // JavaScriptCore.
    bool v;

    VALUE_ISOLATE(objRef,object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return (jboolean)v;
}

NATIVE(JNIJSObject,jlong,callAsConstructor) (PARAMS, jlong objRef, jlongArray args)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;
    jsize len = env->GetArrayLength(args);
    jlong *args_ = env->GetLongArrayElements(args,nullptr);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        int i;
        Local<Value> elements[len];
        for (i=0; i<len; i++) {
            elements[i] = SharedWrap<JSValue>::Shared(args_[i])->Value();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsConstructor(context, len, elements);
        if (value.IsEmpty()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

        if (!exception) {
            out = SharedWrap<JSValue>::New(JSValue::New(object->Context(), value.ToLocalChecked()));
        }
    V8_UNLOCK()

    env->ReleaseLongArrayElements(args, args_, 0);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }

    return out;
}

NATIVE(JNIJSObject,jobjectArray,copyPropertyNames) (PARAMS, jlong objRef) {
    jobjectArray ret;
    size_t length;
    const char **nameArray;

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        Local<Array> names = o->GetPropertyNames(context).ToLocalChecked();
        length = names->Length();
        nameArray = new const char * [length];

        for (size_t i=0; i<length; i++) {
            Local<String> property =
                names->Get(context, i).ToLocalChecked()->ToString(context).ToLocalChecked();
            String::Utf8Value const str(property);
            nameArray[i] = strdup(*str);
        }
    V8_UNLOCK()

    ret = (jobjectArray) env->NewObjectArray(
            length,
            env->FindClass("java/lang/String"),
            env->NewStringUTF(""));
    for (size_t i=0; i<length; i++) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(nameArray[i]));
        delete nameArray[i];
    }
    delete nameArray;

    return ret;
}