//
//  ScriptCompiler.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/8/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "ScriptCompiler.h"

using namespace v8;

ScriptCompiler::CachedData::CachedData(const uint8_t* data, int length,
               BufferPolicy buffer_policy)
{
}

ScriptCompiler::CachedData::~CachedData()
{
}

bool ScriptCompiler::ExternalSourceStream::SetBookmark()
{
    return false;
}
    
void ScriptCompiler::ExternalSourceStream::ResetToBookmark()
{
}


/**
 * Source code which can be streamed into V8 in pieces. It will be parsed
 * while streaming. It can be compiled after the streaming is complete.
 * StreamedSource must be kept alive while the streaming task is ran (see
 * ScriptStreamingTask below).
 */
ScriptCompiler::StreamedSource::StreamedSource(ExternalSourceStream* source_stream, Encoding encoding)
{
}
ScriptCompiler::StreamedSource::~StreamedSource()
{
}
    
// Ownership of the CachedData or its buffers is *not* transferred to the
// caller. The CachedData object is alive as long as the StreamedSource
// object is alive.
const ScriptCompiler::CachedData* ScriptCompiler::StreamedSource::GetCachedData() const
{
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
    return MaybeLocal<Script>();
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
    return MaybeLocal<Function>();
}


Local<String> Message::Get() const
{
    return Local<String>();
}

MaybeLocal<String> Message::GetSourceLine(Local<Context> context) const
{
    return Local<String>();
}

/**
 * Returns the origin for the script from where the function causing the
 * error originates.
 */
ScriptOrigin Message::GetScriptOrigin() const
{
    return ScriptOrigin(Local<Value>());
}

/**
 * Returns the resource name for the script from where the function causing
 * the error originates.
 */
Local<Value> Message::GetScriptResourceName() const
{
    return Local<Value>();
}

/**
 * Exception stack trace. By default stack traces are not captured for
 * uncaught exceptions. SetCaptureStackTraceForUncaughtExceptions allows
 * to change this option.
 */
Local<StackTrace> Message::GetStackTrace() const
{
    return Local<StackTrace>();
}

/**
 * Returns the number, 1-based, of the line where the error occurred.
 */
Maybe<int> Message::GetLineNumber(Local<Context> context) const
{
    return Nothing<int>();
}

/**
 * Returns the index within the script of the first character where
 * the error occurred.
 */
int Message::GetStartPosition() const
{
    return 0;
}

/**
 * Returns the index within the script of the last character where
 * the error occurred.
 */
int Message::GetEndPosition() const
{
    return 0;
}

/**
 * Returns the error level of the message.
 */
int Message::ErrorLevel() const
{
    return 0;
}

/**
 * Returns the index within the line of the first character where
 * the error occurred.
 */
Maybe<int> Message::GetStartColumn(Local<Context> context) const
{
    return Nothing<int>();
}

/**
 * Returns the index within the line of the last character where
 * the error occurred.
 */
Maybe<int> Message::GetEndColumn(Local<Context> context) const
{
    return Nothing<int>();
}

/**
 * Passes on the value set by the embedder when it fed the script from which
 * this Message was generated to V8.
 */
bool Message::IsSharedCrossOrigin() const
{
    return false;
}
bool Message::IsOpaque() const
{
    return false;
}

void Message::PrintCurrentStackTrace(Isolate* isolate, FILE* out)
{
    
}

/**
 * Returns the number, 1-based, of the line for the associate function call.
 * This method will return Message::kNoLineNumberInfo if it is unable to
 * retrieve the line number, or if kLineNumber was not passed as an option
 * when capturing the StackTrace.
 */
int StackFrame::GetLineNumber() const
{
    return 0;
}

/**
 * Returns the 1-based column offset on the line for the associated function
 * call.
 * This method will return Message::kNoColumnInfo if it is unable to retrieve
 * the column number, or if kColumnOffset was not passed as an option when
 * capturing the StackTrace.
 */
int StackFrame::GetColumn() const
{
    return 0;
}

/**
 * Returns the id of the script for the function for this StackFrame.
 * This method will return Message::kNoScriptIdInfo if it is unable to
 * retrieve the script id, or if kScriptId was not passed as an option when
 * capturing the StackTrace.
 */
int StackFrame::GetScriptId() const
{
    return 0;
}

/**
 * Returns the name of the resource that contains the script for the
 * function for this StackFrame.
 */
Local<String> StackFrame::GetScriptName() const
{
    return Local<String>();
}

/**
 * Returns the name of the resource that contains the script for the
 * function for this StackFrame or sourceURL value if the script name
 * is undefined and its source ends with //# sourceURL=... string or
 * deprecated //@ sourceURL=... string.
 */
Local<String> StackFrame::GetScriptNameOrSourceURL() const
{
    return Local<String>();
}

/**
 * Returns the name of the function associated with this stack frame.
 */
Local<String> StackFrame::GetFunctionName() const
{
    return Local<String>();
}

/**
 * Returns whether or not the associated function is compiled via a call to
 * eval().
 */
bool StackFrame::IsEval() const
{
    return false;
}

/**
 * Returns whether or not the associated function is called as a
 * constructor via "new".
 */
bool StackFrame::IsConstructor() const
{
    return false;
}

/**
 * Returns whether or not the associated functions is defined in wasm.
 */
bool StackFrame::IsWasm() const
{
    return false;
}


Local<StackFrame> StackTrace::GetFrame(uint32_t index) const
{
    return Local<StackFrame>();
}

/**
 * Returns the number of StackFrames.
 */
int StackTrace::GetFrameCount() const
{
    return 0;
}

/**
 * Grab a snapshot of the current JavaScript execution stack.
 *
 * \param frame_limit The maximum number of stack frames we want to capture.
 * \param options Enumerates the set of things we will capture for each
 *   StackFrame.
 */
Local<StackTrace> StackTrace::CurrentStackTrace(Isolate* isolate, int frame_limit,
                                                StackTraceOptions options)
{
    return Local<StackTrace>();
}
