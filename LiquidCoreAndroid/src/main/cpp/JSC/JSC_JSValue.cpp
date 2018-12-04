/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSC/Macros.h"
#include "JSC/JSC.h"
#include "JSC/TempException.h"

JS_EXPORT JSType JSValueGetType(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return kJSTypeNull;

    JSType type = kJSTypeUndefined;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        if      (value->IsNull())   type = kJSTypeNull;
        else if (value->IsObject()) type = kJSTypeObject;
        else if (value->IsNumber()) type = kJSTypeNumber;
        else if (value->IsString()) type = kJSTypeString;
        else if (value->IsBoolean())type = kJSTypeBoolean;
    V8_UNLOCK()

    return type;
}

JS_EXPORT bool JSValueIsUndefined(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return false;

    bool v;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        v = value->IsUndefined();
    V8_UNLOCK()

    return v;
}

JS_EXPORT bool JSValueIsNull(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return true;

    bool v;
    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        v = value->IsNull();
    V8_UNLOCK()

    return v;
}

JS_EXPORT bool JSValueIsBoolean(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return false;
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        v = value->IsBoolean();
    V8_UNLOCK()

    return v;
}

JS_EXPORT bool JSValueIsNumber(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return false;
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        v = value->IsNumber();
    V8_UNLOCK()

    return v;
}

JS_EXPORT bool JSValueIsString(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return false;
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        v = value->IsString();
    V8_UNLOCK()

    return v;
}

JS_EXPORT bool JSValueIsObject(JSContextRef ctxRef, JSValueRef valueRef)
{
    if (!valueRef) return false;
    bool v;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        v = value->IsObject();
    V8_UNLOCK()

    return v;
}

JS_EXPORT bool JSValueIsObjectOfClass(JSContextRef ctx, JSValueRef value, JSClassRef jsClass)
{
    if (!value || !jsClass) return false;

    bool v=false;
    VALUE_ISOLATE(CTX(ctx),value,isolate,context,value_)
        MaybeLocal<Object> obj = value_->ToObject(context);
        if (!obj.IsEmpty()) {
            Local<Object> o = obj.ToLocalChecked();
            if (o->InternalFieldCount() > INSTANCE_OBJECT_CLASS) {
                v = ((JSClassRef)o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_CLASS)) == jsClass;
            } else if (o->GetPrototype()->IsObject()) {
                Local<Object> proto = o->GetPrototype()->ToObject(context).ToLocalChecked();
                if (proto->InternalFieldCount() > INSTANCE_OBJECT_CLASS) {
                    v = ((JSClassRef)proto->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_CLASS)) == jsClass;
                }
            }
        }
    V8_UNLOCK()

    return v;
}

/* Comparing values */

JS_EXPORT bool JSValueIsEqual(JSContextRef ctxRef, JSValueRef a, JSValueRef b,
    JSValueRef* exceptionRef)
{
    if (!a && !b) return true;
    if (!a) return JSValueIsNull(ctxRef, b);
    if (!b) return JSValueIsNull(ctxRef, a);

    bool result = false;
    {
        VALUE_ISOLATE(CTX(ctxRef),a,isolate,context,a_)
            TempException exception(exceptionRef);
            Local<Value> b_ = b->L();

            TryCatch trycatch(isolate);

            Maybe<bool> is = a_->Equals(context,b_);
            if (is.IsNothing()) {
                if (trycatch.HasCaught())
                    exception.Set(ctxRef, trycatch.Exception());
                else {
                    TempJSValue e(ctxRef, "Cannot compare values");
                    JSValueRef args[] = {*e};
                    exception.Set(JSObjectMakeError(ctxRef, 1, args, nullptr));
                }

            } else {
                result = is.FromMaybe(result);
            }
        V8_UNLOCK()
    }

    return result;
}

JS_EXPORT bool JSValueIsStrictEqual(JSContextRef ctxRef, JSValueRef a, JSValueRef b)
{
    if (!a && !b) return true;
    if (!a) return JSValueIsNull(ctxRef, b);
    if (!b) return JSValueIsNull(ctxRef, a);

    bool v;
    VALUE_ISOLATE(CTX(ctxRef),a,isolate,context,a_)
        Local<Value> b_ = b->L();
        v = a_->StrictEquals(b_);
    V8_UNLOCK()
    return v;
}

JS_EXPORT bool JSValueIsInstanceOfConstructor(JSContextRef ctxRef, JSValueRef valueRef,
    JSObjectRef constructor, JSValueRef* exceptionRef)
{
    if (!valueRef || !constructor) return false;

    bool is=false;
    V8_ISOLATE_CTX(CTX(ctxRef),isolate,context)
        TempException exception(exceptionRef);

        OpaqueJSString value("value");
        OpaqueJSString ctor("ctor");
        OpaqueJSString fname("__instanceof");
        OpaqueJSString body("return value instanceof ctor;");
        OpaqueJSString source("anonymous");

        JSStringRef paramList[] = { &value, &ctor };
        JSValueRef argList[] = { valueRef, constructor };
        JSObjectRef function = JSObjectMakeFunction(
            ctxRef,
            &fname,
            2,
            paramList,
            &body,
            &source,
            1,
            &exception);
        if (!*exception) {
            JSValueRef is_ = JSObjectCallAsFunction(
                ctxRef,
                function,
                nullptr,
                2,
                argList,
                &exception);
            if (!*exception) {
                is = JSValueToBoolean(ctxRef, is_);
            }
        }
    V8_UNLOCK()

    return is;
}

/* Creating values */

JS_EXPORT JSValueRef JSValueMakeUndefined(JSContextRef ctx)
{
    JSValueRef value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        value = &* OpaqueJSValue::New(ctx,Local<Value>::New(isolate,Undefined(isolate)));
    V8_UNLOCK()

    return value;
}

JS_EXPORT JSValueRef JSValueMakeNull(JSContextRef ctx)
{
    JSValueRef value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        value = &* OpaqueJSValue::New(ctx,Local<Value>::New(isolate,Null(isolate)));
    V8_UNLOCK()

    return value;
}

JS_EXPORT JSValueRef JSValueMakeBoolean(JSContextRef ctx, bool boolean)
{
    JSValueRef value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        value = &* OpaqueJSValue::New(ctx,
            Local<Value>::New(isolate,boolean ? v8::True(isolate):v8::False(isolate)));
    V8_UNLOCK()

    return value;
}

JS_EXPORT JSValueRef JSValueMakeNumber(JSContextRef ctx, double number)
{
    JSValueRef value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        value = &* OpaqueJSValue::New(ctx,Number::New(isolate,number));
    V8_UNLOCK()

    return value;
}

JS_EXPORT JSValueRef JSValueMakeString(JSContextRef ctx, JSStringRef string)
{
    if (!string) return nullptr;

    JSValueRef value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        value = &* OpaqueJSValue::New(ctx,string->Value(isolate));
    V8_UNLOCK()

    return value;
}

/* Converting to and from JSON formatted strings */

JS_EXPORT JSValueRef JSValueMakeFromJSONString(JSContextRef ctx, JSStringRef string)
{
    if (!string || !string->Chars()) return nullptr;
    JSValueRef value = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        MaybeLocal<Value> parsed = JSON::Parse(isolate,
            static_cast<OpaqueJSString*>(string)->Value(isolate));
        if (!parsed.IsEmpty())
            value = &* OpaqueJSValue::New(ctx,parsed.ToLocalChecked());
    V8_UNLOCK()

    return value;
}

JS_EXPORT JSStringRef JSValueCreateJSONString(JSContextRef ctxRef, JSValueRef valueRef,
    unsigned indent, JSValueRef* exceptionRef)
{
    if (!valueRef) return &* OpaqueJSString::New("null");

    OpaqueJSString *value = nullptr;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,inValue)
        TempException exception(exceptionRef);
        TryCatch trycatch(isolate);

        Local<Value> args[] = {
            inValue,
            Local<Value>::New(isolate,Null(isolate)),
            Number::New(isolate, indent)
        };

        // FIXME: I don't understand why this hack works but will fail in a secondary context without
        context->Global()->Get(context, String::NewFromUtf8(isolate, "JSON"));

        Local<Object> json = context->Global()->Get(String::NewFromUtf8(isolate, "JSON"))->ToObject();
        Local<Function> stringify = json->Get(String::NewFromUtf8(isolate, "stringify")).As<Function>();

        MaybeLocal<Value> result = stringify->Call(context, json, 3, args);
        if (result.IsEmpty()) {
            exception.Set(ctxRef, trycatch.Exception());
        } else if (!result.ToLocalChecked()->IsUndefined()) {
            Local<String> string = result.ToLocalChecked()->ToString(context).ToLocalChecked();
            value = &* OpaqueJSString::New(string);
        } else if (exceptionRef) {
            TempJSValue e(ctxRef, "Unserializable value");
            JSValueRef args[] = {*e};
            exception.Set(JSObjectMakeError(ctxRef, 1, args, nullptr));
        }
    V8_UNLOCK()

    return value;
}

/* Converting to primitive values */

JS_EXPORT bool JSValueToBoolean(JSContextRef ctx, JSValueRef valueRef)
{
    if (!valueRef) return false;

    bool ret = false;
    VALUE_ISOLATE(CTX(ctx),valueRef,isolate,context,value)
        MaybeLocal<Boolean> boolean = value->ToBoolean(context);
        if (!boolean.IsEmpty()) {
            ret = boolean.ToLocalChecked()->Value();
        }
    V8_UNLOCK()
    return ret;
}

JS_EXPORT double JSValueToNumber(JSContextRef ctxRef, JSValueRef valueRef, JSValueRef* exceptionRef)
{
    if (!valueRef) return 0;

    double result = __builtin_nan("");
    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        TempException exception(exceptionRef);
        TryCatch trycatch(isolate);

        MaybeLocal<Number> number = value->ToNumber(context);
        if (!number.IsEmpty()) {
            result = number.ToLocalChecked()->Value();
        } else {
            exception.Set(ctxRef, trycatch.Exception());
        }
    V8_UNLOCK()

    return result;
}

JS_EXPORT JSStringRef JSValueToStringCopy(JSContextRef ctxRef, JSValueRef valueRef,
    JSValueRef* exceptionRef)
{
    if (!valueRef) return &* OpaqueJSString::New("null");

    JSStringRef out = nullptr;

    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        TempException exception(exceptionRef);
        TryCatch trycatch(isolate);

        MaybeLocal<String> string = value->ToString(context);
        if (!string.IsEmpty()) {
            String::Utf8Value chars(string.ToLocalChecked());
            out = &* OpaqueJSString::New(*chars);
        } else {
            exception.Set(ctxRef, trycatch.Exception());
        }
    V8_UNLOCK()

    return out;
}

JS_EXPORT JSObjectRef JSValueToObject(JSContextRef ctxRef, JSValueRef valueRef,
    JSValueRef* exceptionRef)
{
    JSObjectRef out = nullptr;
    VALUE_ISOLATE(CTX(ctxRef),valueRef,isolate,context,value)
        TempJSValue null(JSValueMakeNull(ctxRef));
        if (!valueRef) valueRef = *null;

        TempException exception(exceptionRef);
        TryCatch trycatch(isolate);

        MaybeLocal<Object> obj = value->ToObject(context);
        if (!obj.IsEmpty()) {
            out = const_cast<JSObjectRef>(OpaqueJSValue::New(ctxRef, value->ToObject()));
        } else {
            exception.Set(ctxRef, trycatch.Exception());
        }
    V8_UNLOCK()

    return out;
}

/* Garbage collection */
JS_EXPORT void JSValueProtect(JSContextRef ctx, JSValueRef valueRef)
{
    if (valueRef) {
        const_cast<OpaqueJSValue *>(valueRef)->Retain();
    }
}

JS_EXPORT void JSValueUnprotect(JSContextRef ctx, JSValueRef valueRef)
{
    if (valueRef) {
        const_cast<OpaqueJSValue *>(valueRef)->Release(false);
    }
}
