//
// Created by Eric on 11/13/16.
//

#include "JSC.h"

JS_EXPORT JSValueRef JSEvaluateScript(JSContextRef ctx, JSStringRef script_, JSObjectRef thisObject,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception)
{
    // V8 does not have a way to set the 'this' pointer when calling a script directly.
    // To deal with this, we create a function with the script as the body, and then call the
    // function.  This allows us to set 'thisObject'.
    *exception = nullptr;
    JSValueRef ret = nullptr;

    JSObjectRef global = JSContextGetGlobalObject(ctx);
    if (!thisObject)
        thisObject = global;

    OpaqueJSString name("anonymous");
    JSObjectRef func = JSObjectMakeFunction(
        ctx,
        &name,
        0,
        nullptr,
        script_,
        sourceURL ? sourceURL : &name,
        startingLineNumber,
        exception);

    if (!*exception) {
        ret = JSObjectCallAsFunction(
            ctx,
            func,
            thisObject,
            0,
            nullptr,
            exception);
    }
    JSValueUnprotect(ctx, func);
    JSValueUnprotect(ctx, global);

    return ret;
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
            sourceURL->Value(isolate),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Value>  result;
        MaybeLocal<Script> script = Script::Compile(context,script_->Value(isolate),&script_origin);
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
        while(!isolate->IdleNotificationDeadline(1)) {};
    V8_UNLOCK();
}
