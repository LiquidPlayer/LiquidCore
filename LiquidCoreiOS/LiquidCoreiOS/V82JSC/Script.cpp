//
//  Script.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

MaybeLocal<Script> Script::Compile(Local<Context> context, Local<String> source,
                                   ScriptOrigin* origin)
{
    ContextImpl * ctx = static_cast<ContextImpl *>(*context);
    StringImpl * src = reinterpret_cast<StringImpl *>(*source);
    JSStringRef sourceURL = nullptr;
    int startingLineNumber = 0;
    JSValueRef exception = nullptr;
    
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
        
        // HACK!
        Local<Script> s = Local<Script>();
        *(reinterpret_cast<Script **>(&s)) = script;
        
        return MaybeLocal<Script>(s);
    }
}

MaybeLocal<Value> Script::Run(Local<Context> context)
{
    ContextImpl * ctx = static_cast<ContextImpl *>(*context);
    ScriptImpl * script = static_cast<ScriptImpl *>(this);

    JSValueRef exception = nullptr;
    JSValueRef value = JSEvaluateScript(ctx->m_context, script->m_script, nullptr, script->m_sourceURL,
                                        script->m_startingLineNumber, &exception);
    if (!exception) {
        return MaybeLocal<Value>(ValueImpl::New(ctx, value));
    }
    // FIXME: Do something with the exception
    return MaybeLocal<Value>();
}

Local<UnboundScript> Script::GetUnboundScript()
{
    return Local<UnboundScript>();
}


Local<Script> UnboundScript::BindToCurrentContext()
{
    return Local<Script>();
}

int UnboundScript::GetId()
{
    return 0;
}
Local<Value> UnboundScript::GetScriptName()
{
    return Local<Value>();
}

/**
 * Data read from magic sourceURL comments.
 */
Local<Value> UnboundScript::GetSourceURL()
{
    return Local<Value>();
}
/**
 * Data read from magic sourceMappingURL comments.
 */
Local<Value> UnboundScript::GetSourceMappingURL()
{
    return Local<Value>();
}

/**
 * Returns zero based line number of the code_pos location in the script.
 * -1 will be returned if no information available.
 */
int UnboundScript::GetLineNumber(int code_pos)
{
    return 0;
}

