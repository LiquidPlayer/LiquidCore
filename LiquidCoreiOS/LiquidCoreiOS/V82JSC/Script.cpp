//
//  Script.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Script.h"
#include "Context.h"
#include "StringImpl.h"
#include "Value.h"
#include "Utils.h"

using namespace v8;

MaybeLocal<Script> Script::Compile(Local<Context> context, Local<String> source,
                                   ScriptOrigin* origin)
{
    ContextImpl * ctx = static_cast<ContextImpl *>(*context);
    StringImpl * src = static_cast<StringImpl *>(*source);
    JSStringRef sourceURL = nullptr;
    int startingLineNumber = 0;
    JSValueRef exception;
    
    if (origin) {
        if (*origin->ResourceName()) {
            ValueImpl * v = static_cast<ValueImpl *>(*origin->ResourceName());
            sourceURL = JSValueToStringCopy(ctx->m_context, v->m_value, &exception);
        }
        if (*origin->ResourceLineOffset()) {
            // FIXME
        }
    }
    
    bool success = JSCheckScriptSyntax(ctx->m_context, src->m_string, sourceURL, startingLineNumber, &exception);
    if (!success) {
        // FIXME: Do something with exception
        return MaybeLocal<Script>();
    } else {
        ScriptImpl * script = (ScriptImpl *) malloc(sizeof(ScriptImpl));
        memset(script, 0, sizeof(ScriptImpl));
        
        script->m_sourceURL = sourceURL;
        script->m_startingLineNumber = startingLineNumber;
        script->m_script = JSStringRetain(src->m_string);
        
        Local<Script> s = Utils::NewScript(script);
        return MaybeLocal<Script>(s);
    }
}

MaybeLocal<Value> Script::Run(Local<Context> context)
{
    ContextImpl * ctx = static_cast<ContextImpl *>(*context);
    ScriptImpl * script = static_cast<ScriptImpl *>(this);

    JSValueRef exception;
    JSValueRef value = JSEvaluateScript(ctx->m_context, script->m_script, nullptr, script->m_sourceURL,
                                        script->m_startingLineNumber, &exception);
    if (!exception) {
        return MaybeLocal<Value>(ValueImpl::New(ctx, value));
    }
    // FIXME: Do something with the exception
    return MaybeLocal<Value>();
}

