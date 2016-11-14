//
// Created by Eric on 11/13/16.
//

#include "JSC.h"

JS_EXPORT JSValueRef JSEvaluateScript(JSContextRef ctx, JSStringRef script_, JSObjectRef thisObject,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception)
{
    // FIXME: 'thisObject' is ignored
    JSContext *context_ = (JSContext*)(ctx);
    *exception = nullptr;
    JSValueRef out = nullptr;

    V8_ISOLATE(context_->Group(), isolate);
        TryCatch trycatch(isolate);
        Local<Context> context;

        context = context_->Value();
        Context::Scope context_scope_(context);

        ScriptOrigin script_origin(
            sourceURL->Value(),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Value>  result;
        MaybeLocal<Script> script = Script::Compile(context, script_->Value(), &script_origin);
        if (script.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!*exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!*exception) {
            out = JSValue<Value>::New(context_, result.ToLocalChecked());
        }
    V8_UNLOCK();

    return out;
}

JS_EXPORT bool JSCheckScriptSyntax(JSContextRef ctx, JSStringRef script_, JSStringRef sourceURL,
    int startingLineNumber, JSValueRef* exception)
{
    JSContext *context_ = (JSContext*)(ctx);
    *exception = nullptr;
    bool out = false;

    V8_ISOLATE(context_->Group(), isolate);
        TryCatch trycatch(isolate);
        Local<Context> context;

        context = context_->Value();
        Context::Scope context_scope_(context);

        ScriptOrigin script_origin(
            sourceURL->Value(),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Value>  result;
        MaybeLocal<Script> script = Script::Compile(context, script_->Value(), &script_origin);
        if (script.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        } else {
            out = true;
        }
    V8_UNLOCK();

    return out;

}

JS_EXPORT void JSGarbageCollect(JSContextRef ctx)
{
    JSContext *context_ = (JSContext*)(ctx);

    V8_ISOLATE(context_->Group(), isolate);
        while(!isolate->IdleNotificationDeadline(5)) {};
    V8_UNLOCK();
}
