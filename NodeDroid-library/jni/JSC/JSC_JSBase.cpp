//
// Created by Eric on 11/13/16.
//

#include "JSC.h"

#define VALUE(value) ((JSValue<Value> *)(value))

JS_EXPORT JSValueRef JSEvaluateScript(JSContextRef ctx, JSStringRef script_, JSObjectRef thisObject,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exceptionRef)
{
    // V8 does not have a way to set the 'this' pointer when calling a script directly.
    // To deal with this, we create a function with the script as the body, and then call the
    // function.  This allows us to set 'thisObject'.
    if (thisObject) {
        JSStringRef s = JSStringCreateWithUTF8CString("s");
        JSStringRef body = JSStringCreateWithUTF8CString("return eval(s);");
        JSValueRef  eval = JSValueMakeString(ctx, script_);
        JSObjectRef function = JSObjectMakeFunction(ctx, nullptr, 1, &s, body,
            sourceURL, startingLineNumber, exceptionRef);
        JSValueRef ret = JSObjectCallAsFunction(ctx, function, thisObject, 1, &eval, exceptionRef);
        VALUE(eval)->release();
        JSStringRelease(body);
        JSStringRelease(s);
        return ret;
    } else {
        JSValueRef exception = nullptr;
        JSValueRef ret = nullptr;

        OpaqueJSString anonymous("anonymous");

        V8_ISOLATE_CTX(ctx->Context(), isolate, context)
            TryCatch trycatch(isolate);

            ScriptOrigin script_origin(
                sourceURL ? sourceURL->Value(isolate) : anonymous.Value(isolate),
                Integer::New(isolate, startingLineNumber)
            );

            MaybeLocal<Script> script = Script::Compile(context, script_->Value(isolate),
                &script_origin);
            if (script.IsEmpty()) {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            } else {
                MaybeLocal<Value> result = script.ToLocalChecked()->Run(context);
                if (result.IsEmpty()) {
                    exception = JSValue<Value>::New(context_, trycatch.Exception());
                } else {
                    ret = JSValue<Value>::New(context_, result.ToLocalChecked());
                }
            }

            if (exceptionRef) *exceptionRef = exception;
            else if (exception) ((JSValue<Value>*)exception)->release();

        V8_UNLOCK()

        return ret;
    }
}

JS_EXPORT bool JSCheckScriptSyntax(JSContextRef ctx, JSStringRef script_, JSStringRef sourceURL,
    int startingLineNumber, JSValueRef* exceptionRef)
{
    JSValueRef exception = nullptr;
    bool out = false;

    OpaqueJSString anonymous("anonymous");

    V8_ISOLATE_CTX(ctx->Context(), isolate, context)
        TryCatch trycatch(isolate);

        ScriptOrigin script_origin(
            sourceURL ? sourceURL->Value(isolate) : anonymous.Value(isolate),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Value>  result;
        MaybeLocal<Script> script = Script::Compile(context,script_->Value(isolate),&script_origin);
        if (script.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        } else {
            out = true;
        }

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) ((JSValue<Value>*)exception)->release();
    V8_UNLOCK()

    return out;

}

JS_EXPORT void JSGarbageCollect(JSContextRef ctx)
{
    JSContext *context_ = ctx->Context();

    V8_ISOLATE(context_->Group(), isolate)
        while(!isolate->IdleNotificationDeadline(
            group_->Platform()->MonotonicallyIncreasingTime() + 1.0)) {};
    V8_UNLOCK()
}
