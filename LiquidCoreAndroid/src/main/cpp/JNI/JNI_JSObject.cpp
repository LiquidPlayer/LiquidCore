/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */

#include "JNI/JNI.h"
#include "JNI/JSFunction.h"
#include "JNI/JNIJSException.h"

#define VALUE_ISOLATE(objRef,valueRef,isolate,context,value) \
    if (!ISPOINTER(objRef)) { \
        __android_log_assert("!ISPOINTER(##objRef)", "VALUE_ISOLATE", "##ojbRef must be pointer"); \
    } \
    auto (valueRef) = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), objRef); \
    V8_ISOLATE_CTX((valueRef)->Context(),isolate,context); \
    Local<Value> (value) = (valueRef)->Value();

#define V8_ISOLATE_OBJ(objRef,object,isolate,context,o) \
    VALUE_ISOLATE(objRef,object,isolate,context,__v__) \
    Local<Object> (o) = __v__->ToObject(context).ToLocalChecked();

NATIVE(JNIJSObject,jlong,make) (STATIC, jlong context_)
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

NATIVE(JNIJSFunction,jlong,makeFunctionWithCallback) (STATIC, jobject jsfthis, jlong ctx, jstring name)
{
    return SharedWrap<JSValue>::New(JSFunction::New(env, jsfthis, ctx, name));
}

NATIVE(JNIJSFunction,void,setException) (STATIC, jlong funcRef, jlong valueRef)
{
    if (!ISPOINTER(funcRef)) {
        __android_log_assert("!ISPOINTER(a_)", "JNIJSValue.isEqual", "funcRef must be pointer");
    }
    auto func = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), funcRef);
    auto jsfunc = static_cast<JSFunction*>(&* func);
    jsfunc->setException(SharedWrap<JSValue>::Shared(func->Context(), valueRef));
}

NATIVE(JNIJSObject,jlong,makeArray) (STATIC, jlong context_, jlongArray args)
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
            Local<Value> element = SharedWrap<JSValue>::Shared(ctx, args_[i])->Value();
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

NATIVE(JNIJSObject,jlong,makeDate) (STATIC, jlong context_, jlongArray args)
{
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    jlong out = 0;
    jsize len = env->GetArrayLength(args);
    jlong *values = env->GetLongArrayElements(args, nullptr);

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

NATIVE(JNIJSObject,jlong,makeError) (STATIC, jlong context_, jstring message)
{
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    jlong out = 0;
    const char *c_string = env->GetStringUTFChars(message, nullptr);

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<String> str =
            String::NewFromUtf8(isolate, c_string, NewStringType::kNormal).ToLocalChecked();

        out = SharedWrap<JSValue>::New(JSValue::New(ctx, Exception::Error(str)));
    V8_UNLOCK()

    env->ReleaseStringUTFChars(message, c_string);
    return out;
}

NATIVE(JNIJSObject,jlong,makeRegExp) (STATIC, jlong context_, jstring pattern_, jstring flags_)
{
    jlong out = 0;
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    boost::shared_ptr<JSValue> exception;
    const char *_pattern = env->GetStringUTFChars(pattern_, nullptr);
    const char *_flags = env->GetStringUTFChars(flags_, nullptr);

    V8_ISOLATE_CTX(ctx,isolate,context)
        Local<String> pattern =
            String::NewFromUtf8(isolate, _pattern, NewStringType::kNormal).ToLocalChecked();

        unsigned flags = RegExp::Flags::kNone;
        for (size_t i=0; i<strlen(_flags); i++) {
            switch (_flags[i]) {
                case 'g': flags |= RegExp::Flags::kGlobal;     break;
                case 'i': flags |= RegExp::Flags::kIgnoreCase; break;
                case 'm': flags |= RegExp::Flags::kMultiline;  break;
                default: break;
            }
        }

        TryCatch trycatch(isolate);

        MaybeLocal<RegExp> regexp = RegExp::New(context, pattern, (RegExp::Flags)flags);
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

NATIVE(JNIJSObject,jlong,makeFunction) (STATIC, jlong context_, jstring name_,
        jstring func_, jstring sourceURL_, jint startingLineNumber)
{
    jlong out = 0;
    auto ctx = SharedWrap<JSContext>::Shared(context_);
    boost::shared_ptr<JSValue> exception;
    const char *_name = env->GetStringUTFChars(name_, nullptr);
    const char *_func = env->GetStringUTFChars(func_, nullptr);
    const char *_sourceURL = env->GetStringUTFChars(sourceURL_, nullptr);

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

NATIVE(JNIJSObject,jlong,getPrototype) (STATIC, jlong objRef)
{
    jlong out;

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        out = SharedWrap<JSValue>::New(JSValue::New(object->Context(), o->GetPrototype()));
    V8_UNLOCK()

    return out;
}

NATIVE(JNIJSObject,void,setPrototype) (STATIC, jlong objRef, jlong valueRef)
{
    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        o->SetPrototype(context, SharedWrap<JSValue>::Shared(object->Context(), valueRef)->Value());
    V8_UNLOCK()
}

NATIVE(JNIJSObject,jboolean,hasProperty) (STATIC, jlong objRef, jstring propertyName)
{
    bool v;
    const char *c_string = env->GetStringUTFChars(propertyName, nullptr);

    V8_ISOLATE_OBJ(objRef, object,isolate,context,o)
        Maybe<bool> has = o->HasOwnProperty(context, String::NewFromUtf8(isolate, c_string));

        v = has.FromMaybe(false);
    V8_UNLOCK()

    env->ReleaseStringUTFChars(propertyName, c_string);
    return (jboolean)v;
}

NATIVE(JNIJSObject,jlong,getProperty) (STATIC, jlong objRef, jstring propertyName)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;
    const char *c_string = env->GetStringUTFChars(propertyName, nullptr);

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

NATIVE(JNIJSObject,void,setProperty) (STATIC, jlong objRef, jstring propertyName, jlong value, jint attributes)
{
    boost::shared_ptr<JSValue> exception;
    const char *c_string = env->GetStringUTFChars(propertyName, nullptr);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        enum {
            kJSPropertyAttributeReadOnly = 0x2,
            kJSPropertyAttributeDontEnum = 0x4,
            kJSPropertyAttributeDontDelete = 0x8
        };

        unsigned int v8_attr = v8::None;
        if ((unsigned long)attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if ((unsigned long)attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if ((unsigned long)attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        TryCatch trycatch(isolate);

        Maybe<bool> defined = attributes ?
            o->DefineOwnProperty(
                context,
                String::NewFromUtf8(isolate, c_string),
                SharedWrap<JSValue>::Shared(object->Context(), value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, String::NewFromUtf8(isolate, c_string),
                SharedWrap<JSValue>::Shared(object->Context(), value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }
    V8_UNLOCK()

    env->ReleaseStringUTFChars(propertyName, c_string);

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
}

NATIVE(JNIJSObject,jboolean,deleteProperty) (STATIC, jlong objRef, jstring propertyName)
{
    auto out = (jboolean) false;
    boost::shared_ptr<JSValue> exception;
    const char *c_string = env->GetStringUTFChars(propertyName, nullptr);

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

NATIVE(JNIJSObject,jlong,getPropertyAtIndex) (STATIC, jlong objRef, jint propertyIndex)
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

NATIVE(JNIJSObject,void,setPropertyAtIndex) (STATIC, jlong objRef, jint propertyIndex, jlong value)
{
    boost::shared_ptr<JSValue> exception;

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        TryCatch trycatch(isolate);

        Maybe<bool> defined =
            o->Set(context, (uint32_t) propertyIndex,
                   SharedWrap<JSValue>::Shared(object->Context(), value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue::New(object->Context(), trycatch.Exception());
        }

    V8_UNLOCK()

    if (exception) {
        JNIJSException(env, SharedWrap<JSValue>::New(exception)).Throw();
    }
}

NATIVE(JNIJSObject,jboolean,isFunction) (STATIC, jlong objRef)
{
    bool v;

    VALUE_ISOLATE(objRef,object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return (jboolean) v;
}

NATIVE(JNIJSObject,jlong,callAsFunction) (STATIC, jlong objRef, jlong thisObject, jlongArray args)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;
    jsize len = env->GetArrayLength(args);
    jlong *args_ = env->GetLongArrayElements(args,nullptr);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        Local<Value> this_ = thisObject ?
            SharedWrap<JSValue>::Shared(object->Context(), thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        int i;
        Local<Value> elements[len];
        for (i=0; i<len; i++) {
            elements[i] = SharedWrap<JSValue>::Shared(object->Context(), args_[i])->Value();
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

NATIVE(JNIJSObject,jboolean,isConstructor) (STATIC, jlong objRef)
{
    bool v;

    VALUE_ISOLATE(objRef,object,isolate,context,value)
        v = value->IsFunction() && value.As<Function>()->IsConstructor();
    V8_UNLOCK()

    return (jboolean)v;
}

NATIVE(JNIJSObject,jlong,callAsConstructor) (STATIC, jlong objRef, jlongArray args)
{
    jlong out = 0;
    boost::shared_ptr<JSValue> exception;
    jsize len = env->GetArrayLength(args);
    jlong *args_ = env->GetLongArrayElements(args,nullptr);

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        int i;
        Local<Value> elements[len];
        for (i=0; i<len; i++) {
            elements[i] = SharedWrap<JSValue>::Shared(object->Context(), args_[i])->Value();
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

NATIVE(JNIJSObject,jobjectArray,copyPropertyNames) (STATIC, jlong objRef) {
    jobjectArray ret;
    jsize length;
    const char **nameArray;

    V8_ISOLATE_OBJ(objRef,object,isolate,context,o)
        Local<Array> names = o->GetPropertyNames(context).ToLocalChecked();
        length = names->Length();
        nameArray = new const char * [length];

        for (uint32_t i=0; i<(uint32_t)length; i++) {
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
    for (uint32_t i=0; i<(uint32_t)length; i++) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(nameArray[i]));
        delete nameArray[i];
    }
    delete nameArray;

    return ret;
}