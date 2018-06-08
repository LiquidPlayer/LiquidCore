//
//  ScriptCompiler.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/8/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

ScriptCompiler::CachedData::CachedData(const uint8_t* data, int length,
               BufferPolicy buffer_policy)
{
    assert(0);
}

ScriptCompiler::CachedData::~CachedData()
{
    assert(0);
}

bool ScriptCompiler::ExternalSourceStream::SetBookmark()
{
    assert(0);
    return false;
}
    
void ScriptCompiler::ExternalSourceStream::ResetToBookmark()
{
    assert(0);
}


/**
 * Source code which can be streamed into V8 in pieces. It will be parsed
 * while streaming. It can be compiled after the streaming is complete.
 * StreamedSource must be kept alive while the streaming task is ran (see
 * ScriptStreamingTask below).
 */
ScriptCompiler::StreamedSource::StreamedSource(ExternalSourceStream* source_stream, Encoding encoding)
{
    assert(0);
}
ScriptCompiler::StreamedSource::~StreamedSource()
{
    assert(0);
}
    
// Ownership of the CachedData or its buffers is *not* transferred to the
// caller. The CachedData object is alive as long as the StreamedSource
// object is alive.
const ScriptCompiler::CachedData* ScriptCompiler::StreamedSource::GetCachedData() const
{
    assert(0);
    return nullptr;
}

/**
 * Compiles the specified script (context-independent).
 * Cached data as part of the source object can be optionally produced to be
 * consumed later to speed up compilation of identical source scripts.
 *
 * Note that when producing cached data, the source must point to NULL for
 * cached data. When consuming cached data, the cached data must have been
 * produced by the same version of V8.
 *
 * \param source Script source code.
 * \return Compiled script object (context independent; for running it must be
 *   bound to a context).
 */
MaybeLocal<UnboundScript> ScriptCompiler::CompileUnboundScript(
                        Isolate* isolate, Source* source,
                        CompileOptions options)
{
    assert(0);
    return MaybeLocal<UnboundScript>();
}

/**
 * Compiles the specified script (bound to current context).
 *
 * \param source Script source code.
 * \param pre_data Pre-parsing data, as obtained by ScriptData::PreCompile()
 *   using pre_data speeds compilation if it's done multiple times.
 *   Owned by caller, no references are kept when this function returns.
 * \return Compiled script object, bound to the context that was active
 *   when this function was called. When run it will always use this
 *   context.
 */
MaybeLocal<Script> ScriptCompiler::Compile(
                    Local<Context> context, Source* source,
                    CompileOptions options)
{
    Isolate *isolate = V82JSC::ToIsolate(V82JSC::ToContextImpl(context));
    
    ScriptOrigin origin(source->resource_name, source->resource_line_offset,
                        source->resource_column_offset,
                        v8::Boolean::New(isolate, source->GetResourceOptions().IsSharedCrossOrigin()),
                        Local<Integer>(),
                        source->source_map_url,
                        v8::Boolean::New(isolate, source->GetResourceOptions().IsOpaque()),
                        v8::Boolean::New(isolate, source->GetResourceOptions().IsWasm()),
                        v8::Boolean::New(isolate, source->GetResourceOptions().IsModule()));
    return Script::Compile(context, source->source_string, &origin);
}

/**
 * Returns a task which streams script data into V8, or NULL if the script
 * cannot be streamed. The user is responsible for running the task on a
 * background thread and deleting it. When ran, the task starts parsing the
 * script, and it will request data from the StreamedSource as needed. When
 * ScriptStreamingTask::Run exits, all data has been streamed and the script
 * can be compiled (see Compile below).
 *
 * This API allows to start the streaming with as little data as possible, and
 * the remaining data (for example, the ScriptOrigin) is passed to Compile.
 */
ScriptCompiler::ScriptStreamingTask* ScriptCompiler::StartStreamingScript(
                         Isolate* isolate, StreamedSource* source,
                         CompileOptions options)
{
    assert(0);
    return nullptr;
}

/**
 * Compiles a streamed script (bound to current context).
 *
 * This can only be called after the streaming has finished
 * (ScriptStreamingTask has been run). V8 doesn't construct the source string
 * during streaming, so the embedder needs to pass the full source here.
 */
MaybeLocal<Script> ScriptCompiler::Compile(
                        Local<Context> context, StreamedSource* source,
                        Local<String> full_source_string, const ScriptOrigin& origin)
{
    assert(0);
    return MaybeLocal<Script>();
}

/**
 * Return a version tag for CachedData for the current V8 version & flags.
 *
 * This value is meant only for determining whether a previously generated
 * CachedData instance is still valid; the tag has no other meaing.
 *
 * Background: The data carried by CachedData may depend on the exact
 *   V8 version number or current compiler flags. This means that when
 *   persisting CachedData, the embedder must take care to not pass in
 *   data from another V8 version, or the same version with different
 *   features enabled.
 *
 *   The easiest way to do so is to clear the embedder's cache on any
 *   such change.
 *
 *   Alternatively, this tag can be stored alongside the cached data and
 *   compared when it is being used.
 */
uint32_t ScriptCompiler::CachedDataVersionTag()
{
    assert(0);
    return 0;
}

/**
 * This is an unfinished experimental feature, and is only exposed
 * here for internal testing purposes. DO NOT USE.
 *
 * Compile an ES module, returning a Module that encapsulates
 * the compiled code.
 *
 * Corresponds to the ParseModule abstract operation in the
 * ECMAScript specification.
 */
MaybeLocal<Module> ScriptCompiler::CompileModule(Isolate* isolate, Source* source)
{
    assert(0);
    return MaybeLocal<Module>();
}

/**
 * Compile a function for a given context. This is equivalent to running
 *
 * with (obj) {
 *   return function(args) { ... }
 * }
 *
 * It is possible to specify multiple context extensions (obj in the above
 * example).
 */
MaybeLocal<Function> ScriptCompiler::CompileFunctionInContext(
                           Local<Context> context, Source* source, size_t arguments_count,
                           Local<String> arguments[], size_t context_extension_count,
                           Local<Object> context_extensions[])
{
    assert(0);
    return MaybeLocal<Function>();
}

