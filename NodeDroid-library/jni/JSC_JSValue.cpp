//
// Created by Eric on 11/5/16.
//

#include "JSC.h"

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> value = (reinterpret_cast<JSValue<Value>*>(valueRef))->Value()
#define VALUE(value) ((JSValue<Value> *)(value))
#define CTX(ctx)     ((JSContext *)(ctx))

JS_EXPORT JSType JSValueGetType(JSContextRef ctxRef, JSValueRef valueRef)
{
    JSType type = kJSTypeUndefined;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        if      (value->IsNull())   type = kJSTypeNull;
        else if (value->IsObject()) type = kJSTypeObject;
        else if (value->IsNumber()) type = kJSTypeNumber;
        else if (value->IsString()) type = kJSTypeString;
        else if (value->IsBoolean())type = kJSTypeBoolean;
    V8_UNLOCK();

    return type;
}

JS_EXPORT bool JSValueIsUndefined(JSContextRef ctxRef, JSValueRef valueRef)
{
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        v = value->IsUndefined();
    V8_UNLOCK();

    return v;
}

JS_EXPORT bool JSValueIsNull(JSContextRef ctxRef, JSValueRef valueRef)
{
    bool v;
    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        v = value->IsNull();
    V8_UNLOCK();

    return v;
}

JS_EXPORT bool JSValueIsBoolean(JSContextRef ctxRef, JSValueRef valueRef)
{
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        v = value->IsBoolean();
    V8_UNLOCK();

    return v;
}

JS_EXPORT bool JSValueIsNumber(JSContextRef ctxRef, JSValueRef valueRef)
{
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        v = value->IsNumber();
    V8_UNLOCK();

    return v;
}

JS_EXPORT bool JSValueIsString(JSContextRef ctxRef, JSValueRef valueRef)
{
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        v = value->IsString();
    V8_UNLOCK();

    return v;
}

JS_EXPORT bool JSValueIsObject(JSContextRef ctxRef, JSValueRef valueRef)
{
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        v = value->IsObject();
    V8_UNLOCK();

    return v;
}

JS_EXPORT bool JSValueIsObjectOfClass(JSContextRef ctx, JSValueRef value, JSClassRef jsClass)
{
    bool v=false;
    VALUE_ISOLATE(CTX(ctx),VALUE(value),isolate,context,value_);
        MaybeLocal<Object> obj = value_->ToObject(context);
        if (!obj.IsEmpty()) {
            Local<Object> o = obj.ToLocalChecked();
            if (o->InternalFieldCount() > 0) {
                v = ((JSClassRef)o->GetAlignedPointerFromInternalField(0)) == jsClass;
            }
        }
    V8_UNLOCK();

    return v;
}

/* Comparing values */

JS_EXPORT bool JSValueIsEqual(JSContextRef ctxRef, JSValueRef a, JSValueRef b,
    JSValueRef* exception)
{
    bool result = false;
    *exception = nullptr;
    {
        VALUE_ISOLATE(CTX(ctxRef),VALUE(a),isolate,context,a_);
            Local<Value> b_ = (reinterpret_cast<JSValue<Value>*>(VALUE(b)))->Value();

            TryCatch trycatch(isolate);

            Maybe<bool> is = a_->Equals(context,b_);
            if (is.IsNothing()) {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            } else {
                result = is.FromMaybe(result);
            }
        V8_UNLOCK();
    }

    return result;
}

JS_EXPORT bool JSValueIsStrictEqual(JSContextRef ctxRef, JSValueRef a, JSValueRef b)
{
    bool v;
    VALUE_ISOLATE(CTX(ctxRef),VALUE(a),isolate,context,a_);
        Local<Value> b_ = (reinterpret_cast<JSValue<Value>*>(VALUE(b)))->Value();
        v = a_->StrictEquals(b_);
    V8_UNLOCK();
    return v;
}

JS_EXPORT bool JSValueIsInstanceOfConstructor(JSContextRef ctxRef, JSValueRef valueRef,
    JSObjectRef constructor, JSValueRef* exception)
{
    bool is=false;
    V8_ISOLATE_CTX(CTX(ctxRef),isolate,context);
        JSStringRef paramList[] = { JSStringCreateWithUTF8CString("value"),
            JSStringCreateWithUTF8CString("ctor") };
        JSStringRef fname = JSStringCreateWithUTF8CString("__instanceof");
        JSStringRef body = JSStringCreateWithUTF8CString("return value instanceof ctor;");
        JSStringRef source = JSStringCreateWithUTF8CString("anonymous");
        JSValueRef argList[] = { valueRef, constructor };
        JSObjectRef function = JSObjectMakeFunction(
            ctxRef,
            fname,
            2,
            paramList,
            body,
            source,
            1,
            exception);
        if (!*exception) {
            JSValueRef is_ = JSObjectCallAsFunction(
                ctxRef,
                function,
                nullptr,
                2,
                argList,
                exception);
            if (!*exception) {
                is = JSValueToBoolean(ctxRef, is_);
            }
        }
        paramList[0]->release();
        paramList[1]->release();
        fname->release();
        body->release();
        source->release();
    V8_UNLOCK();

    return is;
}

/* Creating values */

JS_EXPORT JSValueRef JSValueMakeUndefined(JSContextRef ctx)
{
    JSValue<Value> *value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        value = JSValue<Value>::New(context_,Local<Value>::New(isolate,Undefined(isolate)));
    V8_UNLOCK();

    return value;
}

JS_EXPORT JSValueRef JSValueMakeNull(JSContextRef ctx)
{
    JSValue<Value> *value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        value = JSValue<Value>::New(context_,Local<Value>::New(isolate,Null(isolate)));
    V8_UNLOCK();

    return value;
}

JS_EXPORT JSValueRef JSValueMakeBoolean(JSContextRef ctx, bool boolean)
{
    JSValue<Value> *value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        value = JSValue<Value>::New(context_,
            Local<Value>::New(isolate,boolean ? v8::True(isolate):v8::False(isolate)));
    V8_UNLOCK();

    return value;
}

JS_EXPORT JSValueRef JSValueMakeNumber(JSContextRef ctx, double number)
{
    JSValue<Value> *value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        value = JSValue<Value>::New(context_,Number::New(isolate,number));
    V8_UNLOCK();

    return value;
}

JS_EXPORT JSValueRef JSValueMakeString(JSContextRef ctx, JSStringRef string)
{
    JSValue<Value> *value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        value = JSValue<Value>::New(context_,static_cast<OpaqueJSString*>(string)->Value(isolate));
    V8_UNLOCK();

    return value;
}

/* Converting to and from JSON formatted strings */

JS_EXPORT JSValueRef JSValueMakeFromJSONString(JSContextRef ctx, JSStringRef string)
{
    JSValue<Value> *value = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        MaybeLocal<Value> parsed = JSON::Parse(isolate,
            static_cast<OpaqueJSString*>(string)->Value(isolate));
        if (!parsed.IsEmpty())
            value = JSValue<Value>::New(context_,parsed.ToLocalChecked());

        if (!value) {
            value = JSValue<Value>::New(context_,Local<Value>::New(isolate,Undefined(isolate)));
        }
    V8_UNLOCK();

    return value;
}

JS_EXPORT JSStringRef JSValueCreateJSONString(JSContextRef ctxRef, JSValueRef valueRef,
    unsigned indent, JSValueRef* exception)
{
    *exception = nullptr;
    OpaqueJSString *value = nullptr;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,inValue);
        Local<Object> json = context->Global()->Get(String::NewFromUtf8(isolate, "JSON"))->ToObject();
        Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();

        Local<Value> result = stringify->Call(json, 1, &inValue);
        Local<String> string = result->ToString(context).ToLocalChecked();

        value = new OpaqueJSString(string);
    V8_UNLOCK();

    return value;
}

/* Converting to primitive values */

JS_EXPORT bool JSValueToBoolean(JSContextRef ctx, JSValueRef valueRef)
{
    bool ret = false;
    VALUE_ISOLATE(CTX(ctx),VALUE(valueRef),isolate,context,value);
        MaybeLocal<Boolean> boolean = value->ToBoolean(context);
        if (!boolean.IsEmpty()) {
            ret = boolean.ToLocalChecked()->Value();
        }
    V8_UNLOCK();
    return ret;
}

JS_EXPORT double JSValueToNumber(JSContextRef ctxRef, JSValueRef valueRef, JSValueRef* exception)
{
    double result = 0.0;
    *exception = nullptr;
    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        TryCatch trycatch(isolate);

        MaybeLocal<Number> number = value->ToNumber(context);
        if (!number.IsEmpty()) {
            result = number.ToLocalChecked()->Value();
        } else {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }
    V8_UNLOCK();

    return result;
}

JS_EXPORT JSStringRef JSValueToStringCopy(JSContextRef ctxRef, JSValueRef valueRef, JSValueRef* exception)
{
    *exception = nullptr;
    JSStringRef out = nullptr;

    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        TryCatch trycatch(isolate);

        MaybeLocal<String> string = value->ToString(context);
        if (!string.IsEmpty()) {
            String::Utf8Value const str(string.ToLocalChecked());
            out = JSStringCreateWithUTF8CString((char*)*str);
        } else {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }
    V8_UNLOCK();
    return out;
}

JS_EXPORT JSObjectRef JSValueToObject(JSContextRef ctxRef, JSValueRef valueRef,
    JSValueRef* exception)
{
    *exception = nullptr;
    JSValue<Value> *out = nullptr;
    VALUE_ISOLATE(CTX(ctxRef),VALUE(valueRef),isolate,context,value);
        TryCatch trycatch(isolate);

        MaybeLocal<Object> obj = value->ToObject(context);
        if (!obj.IsEmpty()) {
            out = JSValue<Value>::New(context_, value->ToObject());

        } else {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

    V8_UNLOCK();
    return out;
}

/* Garbage collection */
JS_EXPORT void JSValueProtect(JSContextRef ctx, JSValueRef valueRef)
{
    JSValue<Value> *value = static_cast<JSValue<Value>*>(VALUE(valueRef));
    value->retain();
}

JS_EXPORT void JSValueUnprotect(JSContextRef ctx, JSValueRef valueRef)
{
    JSValue<Value> *value = static_cast<JSValue<Value>*>(VALUE(valueRef));
#ifdef DEBUG_RETAINER
    Retainer::m_debug_mutex.lock();
    bool found = (std::find(Retainer::m_debug.begin(),
        Retainer::m_debug.end(), value) != Retainer::m_debug.end());
    Retainer::m_debug_mutex.unlock();
    if (!found) {
        __android_log_assert(found ? "FAIL" : nullptr,
            "unprotect", "Attempting to unprotect a dead reference!");
    }
#endif
    value->release();
}
