/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSC/JSC.h"
#include "JSC/TempException.h"

JS_EXPORT JSValueRef JSEvaluateScript(JSContextRef ctx, JSStringRef script_, JSObjectRef thisObject,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exceptionRef)
{
    // V8 does not have a way to set the 'this' pointer when calling a script directly.
    // To deal with this, we create a function with the script as the body, and then call the
    // function.  This allows us to set 'thisObject'.
    if (thisObject) {
        OpaqueJSString s("s"); JSStringRef args[] = { &s };
        OpaqueJSString body("return eval(s);");
        TempJSValue eval(JSValueMakeString(ctx, script_)); JSValueRef callargs[] = { *eval };
        TempJSValue function(JSObjectMakeFunction(ctx, nullptr, 1, args, &body,
            sourceURL, startingLineNumber, exceptionRef));
        return JSObjectCallAsFunction(ctx, const_cast<JSObjectRef>(*function), thisObject,
            1, callargs, exceptionRef);
    } else {
        JSValueRef ret = nullptr;

        V8_ISOLATE_CTX(ctx->Context(), isolate, context)
            TempException exception(exceptionRef);
            OpaqueJSString anonymous("anonymous");
            TryCatch trycatch(isolate);

            ScriptOrigin script_origin(
                sourceURL ? sourceURL->Value(isolate) : anonymous.Value(isolate),
                Integer::New(isolate, startingLineNumber)
            );

            MaybeLocal<Script> script = Script::Compile(context, script_->Value(isolate),
                &script_origin);
            if (script.IsEmpty()) {
                exception.Set(ctx, trycatch.Exception());
            } else {
                MaybeLocal<Value> result = script.ToLocalChecked()->Run(context);
                if (result.IsEmpty()) {
                    exception.Set(ctx, trycatch.Exception());
                } else {
                    ret = &* OpaqueJSValue::New(ctx, result.ToLocalChecked());
                }
            }
        V8_UNLOCK()

        return ret;
    }
}

JS_EXPORT bool JSCheckScriptSyntax(JSContextRef ctx, JSStringRef script_, JSStringRef sourceURL,
    int startingLineNumber, JSValueRef* exceptionRef)
{
    bool out = false;

    V8_ISOLATE_CTX(ctx->Context(), isolate, context)
        TempException exception(exceptionRef);
        OpaqueJSString anonymous("anonymous");
        TryCatch trycatch(isolate);

        ScriptOrigin script_origin(
            sourceURL ? sourceURL->Value(isolate) : anonymous.Value(isolate),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Value>  result;
        MaybeLocal<Script> script = Script::Compile(context,script_->Value(isolate),&script_origin);
        if (script.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        } else {
            out = true;
        }
    V8_UNLOCK()

    return out;

}

JS_EXPORT void JSGarbageCollect(JSContextRef ctx)
{
    const_cast<OpaqueJSContext *>(ctx)->ForceGC();
}
