//
// JSC_JSBase.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
#include "JSC.h"

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
                    ret = OpaqueJSValue::New(ctx, result.ToLocalChecked());
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
