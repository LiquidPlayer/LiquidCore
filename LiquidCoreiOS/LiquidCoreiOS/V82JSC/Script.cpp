//
//  Script.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;
#define H V82JSC_HeapObject

MaybeLocal<Script> Script::Compile(Local<Context> context, Local<String> source,
                                   ScriptOrigin* origin)
{
    ContextImpl * ctx = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctx);
    Isolate *isolate = V82JSC::ToIsolate(iso);
    JSValueRef src = V82JSC::ToJSValueRef<String>(source, context);
    JSStringRef s = JSValueToStringCopy(ctx->m_ctxRef, src, 0);
    JSStringRef sourceURL = JSStringCreateWithUTF8CString("[undefined]");
    int startingLineNumber = 1;
    LocalException exception(iso);

    if (origin) {
        if (!origin->ResourceName().IsEmpty()) {
            JSValueRef v = V82JSC::ToJSValueRef(origin->ResourceName(), context);
            JSStringRelease(sourceURL);
            sourceURL = JSValueToStringCopy(ctx->m_ctxRef, v, &exception);
        }
    }
    
    bool success = !exception.ShouldThow() && JSCheckScriptSyntax(ctx->m_ctxRef, s, sourceURL, startingLineNumber, &exception);
    if (!success) {
        if (s) JSStringRelease(s);
        return MaybeLocal<Script>();
    } else {
        ScriptImpl *script = static_cast<ScriptImpl *>(H::HeapAllocator::Alloc(iso, iso->m_script_map));
        
        if (origin) {
            script->resource_name.Reset(isolate, origin->ResourceName());
            script->resource_line_offset.Reset(isolate, origin->ResourceLineOffset());
            script->resource_column_offset.Reset(isolate, origin->ResourceColumnOffset());
            script->resource_is_shared_cross_origin = origin->Options().IsSharedCrossOrigin();
            script->script_id.Reset(isolate, origin->ScriptID());
            script->source_map_url.Reset(isolate, origin->SourceMapUrl());
            script->resource_is_opaque = origin->Options().IsOpaque();
            script->is_wasm = origin->Options().IsWasm();
            script->is_module = origin->Options().IsModule();
        }
        
        JSValueRef surl = V82JSC::exec
            (V82JSC::ToContextRef(context),
             "return (/\\/\\/[#@] *sourceURL[\\s]*=([A-Za-z0-9_]*)\\s*$/gm.exec(_1)||['',null])[1]",
             1, &src);
        if (!JSValueIsNull(V82JSC::ToContextRef(context), surl)) {
            JSStringRelease(sourceURL);
            sourceURL = JSValueToStringCopy(V82JSC::ToContextRef(context), surl, 0);
            JSStringRetain(sourceURL);
        }

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
    Context::Scope context_scope(context);

    Local<Script> local = V82JSC::CreateLocal<Script>(&iso->ii, script);
    iso->m_running_scripts.push(local);

    MaybeLocal<Value> ret;
    {
        LocalException exception(iso);
        JSValueRef value = JSEvaluateScript(ctx->m_ctxRef, script->m_script, nullptr, script->m_sourceURL,
                                            script->m_startingLineNumber, &exception);
        if (!exception.ShouldThow()) {
            ret = ValueImpl::New(ctx, value);
        }
    }

    iso->m_running_scripts.pop();
    
    return ret;
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

