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
    ContextImpl * ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    JSValueRef src = V82JSC::ToJSValueRef<String>(source, context);
    JSStringRef s = JSValueToStringCopy(ctx->m_ctxRef, src, 0);
    JSStringRef sourceURL = nullptr;
    int startingLineNumber = 0;
    LocalException exception(iso);

    if (origin) {
        if (*origin->ResourceName()) {
            ValueImpl * v = V82JSC::ToImpl<ValueImpl>(origin->ResourceName());
            sourceURL = JSValueToStringCopy(ctx->m_ctxRef, v->m_value, &exception);
        }
        if (*origin->ResourceLineOffset()) {
            // FIXME
        }
    }
    
    bool success = !exception.ShouldThow() && JSCheckScriptSyntax(ctx->m_ctxRef, s, sourceURL, startingLineNumber, &exception);
    if (!success) {
        JSStringRelease(s);
        return MaybeLocal<Script>();
    } else {
        ScriptImpl *script = static_cast<ScriptImpl *>(HeapAllocator::Alloc(iso, sizeof(ScriptImpl)));
        V82JSC::Map(script)->set_instance_type(internal::SCRIPT_TYPE);

        script->m_sourceURL = sourceURL;
        script->m_startingLineNumber = startingLineNumber;
        script->m_script = s;
        
        return V82JSC::CreateLocal<Script>(V82JSC::ToIsolate(iso), script);
    }
}

MaybeLocal<Value> Script::Run(Local<Context> context)
{
    ContextImpl * ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    ScriptImpl * script = V82JSC::ToImpl<ScriptImpl, Script>(this);

    LocalException exception(iso);
    JSValueRef value = JSEvaluateScript(ctx->m_ctxRef, script->m_script, nullptr, script->m_sourceURL,
                                        script->m_startingLineNumber, &exception);
    if (!exception.ShouldThow()) {
        return MaybeLocal<Value>(ValueImpl::New(ctx, value));
    }

    return MaybeLocal<Value>();
}

Local<UnboundScript> Script::GetUnboundScript()
{
    assert(0);
    return Local<UnboundScript>();
}


Local<Script> UnboundScript::BindToCurrentContext()
{
    assert(0);
    return Local<Script>();
}

int UnboundScript::GetId()
{
    assert(0);
    return 0;
}
Local<Value> UnboundScript::GetScriptName()
{
    assert(0);
    return Local<Value>();
}

/**
 * Data read from magic sourceURL comments.
 */
Local<Value> UnboundScript::GetSourceURL()
{
    assert(0);
    return Local<Value>();
}
/**
 * Data read from magic sourceMappingURL comments.
 */
Local<Value> UnboundScript::GetSourceMappingURL()
{
    assert(0);
    return Local<Value>();
}

/**
 * Returns zero based line number of the code_pos location in the script.
 * -1 will be returned if no information available.
 */
int UnboundScript::GetLineNumber(int code_pos)
{
    assert(0);
    return 0;
}

