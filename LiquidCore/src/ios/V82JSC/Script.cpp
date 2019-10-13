/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <thread>
#include "V82JSC.h"
#include "StringImpl.h"
#include "Script.h"

using namespace V82JSC;
using namespace v8;

MaybeLocal<v8::Script> v8::Script::Compile(Local<Context> context, Local<String> source,
                                   ScriptOrigin* origin)
{
    Isolate *isolate = ToIsolate(ToContextImpl(context));
    ScriptOrigin default_origin(Undefined(isolate));
    if (!origin) origin = &default_origin;
    ScriptCompiler::Source src(source, *origin);
    return ScriptCompiler::Compile(context, &src);
}

MaybeLocal<v8::Value> v8::Script::Run(Local<Context> context)
{
    auto impl = ToImpl<V82JSC::Script>(this);
    IsolateImpl* iso = impl->GetIsolate();
    Isolate *isolate = ToIsolate(iso);
    EscapableHandleScope scope(isolate);
    auto thread = IsolateImpl::PerThreadData::Get(iso);

    Local<Context> bound_context = impl->m_context.Get(isolate);
    Context::Scope context_scope(bound_context);
    JSContextRef ctx = ToContextRef(bound_context);
    Local<Script> local = CreateLocal<Script>(&iso->ii, impl);

    auto unbound = ToImpl<V82JSC::UnboundScript>(impl->m_unbound_script.Get(isolate));
    
    // Check if there are pending interrupts before even executing
    IsolateImpl::PollForInterrupts(ctx, iso);

    if (thread->m_callback_depth == 0 && isolate->GetMicrotasksPolicy() == MicrotasksPolicy::kAuto) {
        thread->m_callback_depth++;
        isolate->RunMicrotasks();
    } else {
        thread->m_callback_depth ++;
    }
    thread->m_running_scripts.push(local);
    MaybeLocal<Value> ret;
    {
        LocalException exception(iso);
        
        if (iso->m_disallow_js) {
            if (iso->m_on_failure == Isolate::DisallowJavascriptExecutionScope::OnFailure::CRASH_ON_FAILURE) {
                FATAL("Javascript execution disallowed");
            } else {
                *(&exception) = exec(ctx, "return new Error('Javascript execution disallowed')", 0, nullptr);
            }
        } else {
            for (auto i=iso->m_before_call_callbacks.begin(); i!=iso->m_before_call_callbacks.end(); ++i) {
                (*i)(isolate);
            }
            
            int line_offset = unbound->m_resource_line_offset.IsEmpty() ? 1 : (int)unbound->m_resource_line_offset.Get(isolate)->Value() + 1;
            JSValueRef value;
#ifdef USE_JAVASCRIPTCORE_PRIVATE_API
            if (line_offset > 1) {
                // This is a hack.  Line offset in JSScriptScriptCreateFromString() does not work.  Schade.
#endif
                JSString url;
                if (!unbound->m_sourceURL.IsEmpty()) {
                    Local<Value> v = unbound->m_sourceURL.Get(isolate);
                    if (!v->IsUndefined())
                        url = JSValueToStringCopy(ctx, ToJSValueRef(v, bound_context), 0);
                }
                if (!url && !unbound->m_resource_name.IsEmpty()) {
                    Local<Value> v = unbound->m_resource_name.Get(isolate);
                    if (!v->IsUndefined())
                        url = JSValueToStringCopy(ctx, ToJSValueRef(v, bound_context), 0);
                }
                if (!url) {
                    url = "[undefined]";
                }

                value = JSEvaluateScript(ctx, unbound->m_script_string, JSContextGetGlobalObject(ctx),
                                         *url, line_offset, &exception);
#ifdef USE_JAVASCRIPTCORE_PRIVATE_API
            } else {
                value = JSCPrivate::JSScriptEvaluate(ctx, unbound->m_script, JSContextGetGlobalObject(ctx), &exception);
            }
#endif
            if (!exception.ShouldThrow()) {
                ret = V82JSC::Value::New(ToContextImpl(bound_context), value);
            }
        }
    }
    thread->m_running_scripts.pop();
    thread->m_callback_depth --;

    if (thread->m_callback_depth == 0) {
        for (auto i=iso->m_call_completed_callbacks.begin(); i!=iso->m_call_completed_callbacks.end(); ++i) {
            thread->m_callback_depth++;
            (*i)(ToIsolate(iso));
            thread->m_callback_depth--;
        }
    }

    if (ret.IsEmpty()) return ret;
    return scope.Escape(ret.ToLocalChecked());
}

Local<v8::UnboundScript> v8::Script::GetUnboundScript()
{
    auto impl = ToImpl<V82JSC::Script>(this);
    Isolate *isolate = ToIsolate(impl->GetIsolate());
    return impl->m_unbound_script.Get(isolate);
}

Local<v8::Script> v8::UnboundScript::BindToCurrentContext()
{
    auto impl = ToImpl<V82JSC::UnboundScript>(this);
    IsolateImpl *iso = impl->GetIsolate();
    Isolate *isolate = ToIsolate(iso);
    
    auto scr = static_cast<V82JSC::Script*>(HeapAllocator::Alloc(iso, iso->m_script_map));

    scr->m_unbound_script.Reset(isolate, CreateLocal<UnboundScript>(&iso->ii, impl));
    scr->m_context.Reset(isolate, isolate->GetCurrentContext());
    
    return CreateLocal<Script>(&iso->ii, scr);
}

int v8::UnboundScript::GetId()
{
    auto impl = ToImpl<V82JSC::UnboundScript>(this);
    if (impl->m_id.IsEmpty()) {
        return kNoScriptId;
    }
    return (int)impl->m_id.Get(ToIsolate(impl->GetIsolate()))->Value();
}
Local<v8::Value> v8::UnboundScript::GetScriptName()
{
    auto impl = ToImpl<V82JSC::UnboundScript>(this);
    return impl->m_resource_name.Get(ToIsolate(impl->GetIsolate()));
}

/**
 * Data read from magic sourceURL comments.
 */
Local<v8::Value> v8::UnboundScript::GetSourceURL()
{
    auto impl = ToImpl<V82JSC::UnboundScript>(this);
    return impl->m_sourceURL.Get(ToIsolate(impl->GetIsolate()));
}
/**
 * Data read from magic sourceMappingURL comments.
 */
Local<v8::Value> v8::UnboundScript::GetSourceMappingURL()
{
    auto impl = ToImpl<V82JSC::UnboundScript>(this);
    return impl->m_sourceMappingURL.Get(ToIsolate(impl->GetIsolate()));
}

/**
 * Returns zero based line number of the code_pos location in the script.
 * -1 will be returned if no information available.
 */
int v8::UnboundScript::GetLineNumber(int code_pos)
{
    if (code_pos == 0) {
        auto impl = ToImpl<V82JSC::UnboundScript>(this);
        Local<Integer> line = impl->m_resource_line_offset.Get(ToIsolate(impl->GetIsolate()));
        if (!line.IsEmpty()) {
            return (int) line->Value();
        }
    }
    return -1;
}

ScriptCompiler::CachedData::CachedData(const uint8_t* data, int length,
                                       BufferPolicy buffer_policy)
{
    this->data = data;
    this->length = length;
    this->buffer_policy = buffer_policy;
    this->rejected = false;
}

ScriptCompiler::CachedData::~CachedData()
{
//    if (data && buffer_policy == BufferPolicy::BufferNotOwned) delete data;
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

struct internal::ScriptStreamingData
{
    ScriptCompiler::ExternalSourceStream* source_stream_;
    ScriptCompiler::StreamedSource::Encoding encoding_;
    ScriptCompiler::CachedData *cached_data_;
};

class V82JSCStreamingTask : public ScriptCompiler::ScriptStreamingTask
{
public:
    V82JSCStreamingTask(ScriptCompiler::StreamedSource* streamed_source) :
        source_stream_(streamed_source->impl()->source_stream_),
        encoding_(streamed_source->impl()->encoding_),
        streamed_source_(streamed_source->impl())
    {
    }
    virtual void Run()
    {
        StreamingTask(this);
    }

    static void StreamingTask(V82JSCStreamingTask *this_)
    {
        const uint8_t *buffer;
        uint8_t *src = nullptr;
        size_t total_size = 0;
        while (true) {
            size_t size = this_->source_stream_->GetMoreData(&buffer);
            if (size == 0) break;
            if (src == nullptr) src = (uint8_t *) malloc(size+1);
            else src = (uint8_t*) realloc(src, total_size + size+1);
            memcpy(&src[total_size], buffer, size);
            total_size += size;
        }
        if (src) src[total_size--] = 0;
        this_->streamed_source_->cached_data_ = new ScriptCompiler::CachedData(src, int(total_size));
    }
    ScriptCompiler::ExternalSourceStream* source_stream_;
    ScriptCompiler::StreamedSource::Encoding encoding_;
    internal::ScriptStreamingData *streamed_source_;
};

/**
 * Source code which can be streamed into V8 in pieces. It will be parsed
 * while streaming. It can be compiled after the streaming is complete.
 * StreamedSource must be kept alive while the streaming task is ran (see
 * ScriptStreamingTask below).
 */
ScriptCompiler::StreamedSource::StreamedSource(ExternalSourceStream* source_stream, Encoding encoding)
{
    impl_ = new internal::ScriptStreamingData();
    impl_->source_stream_ = source_stream;
    impl_->encoding_ = encoding;
}
ScriptCompiler::StreamedSource::~StreamedSource()
{
    delete impl_;
}

// Ownership of the CachedData or its buffers is *not* transferred to the
// caller. The CachedData object is alive as long as the StreamedSource
// object is alive.
const ScriptCompiler::CachedData* ScriptCompiler::StreamedSource::GetCachedData() const
{
    return impl()->cached_data_;
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
MaybeLocal<v8::UnboundScript> ScriptCompiler::CompileUnboundScript(Isolate* isolate, Source* source,
                                                               CompileOptions options,
                                                               NoCacheReason no_cache_reason)
{
    IsolateImpl* iso = ToIsolateImpl(isolate);
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = iso->m_nullContext.Get(isolate);
    JSContextRef ctx = ToContextRef(context);
    
    auto impl = static_cast<V82JSC::UnboundScript*>(HeapAllocator::Alloc(iso, iso->m_unbound_script_map));
    
    JSString url;
    JSString src;
    if (!source->source_string.IsEmpty()) {
        JSValueRef s = ToJSValueRef(source->source_string, context);
        // FIXME: Would be nice to figure out how to do this without a copy
        src = JSValueToStringCopy(ctx, s, 0);
        
        JSValueRef surl = exec(ctx,
                                       "var re = /\\/\\/[#@] sourceURL=[\\s]*([A-Za-z0-9_\\.]*)\\s*\(\\n|$)/g;"
                                       "var match = re.exec(_1);"
                                       "var out = null;"
                                       "while (match != null) {"
                                       "    out = match[1];"
                                       "    match = re.exec(_1);"
                                       "}"
                                       "return out;",
                                       1, &s);
        if (!JSValueIsNull(ctx, surl)) {
            url = JSValueToStringCopy(ctx, surl, 0);
            impl->m_sourceURL.Reset(isolate, V82JSC::Value::New(ToContextImpl(context), surl));
        } else {
            impl->m_sourceURL.Reset(isolate, Undefined(isolate));
        }
        JSValueRef smurl = exec(ctx,
                                        "var re = /\\/\\/[#@] sourceMappingURL=[\\s]*([A-Za-z0-9_\\.]*)\\s*(\\n|$)/g;"
                                        "var match = re.exec(_1);"
                                        "var out = null;"
                                        "while (match != null) {"
                                        "    out = match[1];"
                                        "    match = re.exec(_1);"
                                        "}"
                                        "return out;",
                                        1, &s);
        if (!JSValueIsNull(ctx, smurl)) {
            impl->m_sourceMappingURL.Reset(isolate, V82JSC::Value::New(ToContextImpl(context), smurl));
        } else if (!source->source_map_url.IsEmpty()) {
            impl->m_sourceMappingURL.Reset(isolate, source->source_map_url);
        } else {
            impl->m_sourceMappingURL.Reset(isolate, Undefined(isolate));
        }
    }
    if (!source->resource_name.IsEmpty()) {
        impl->m_resource_name.Reset(isolate, source->resource_name);
        if (!url) {
            JSValueRef s = ToJSValueRef(source->resource_name, context);
            url = JSValueToStringCopy(ctx, s, 0);
        }
    }
    int startingLineNumber = 1;
    if (!source->resource_line_offset.IsEmpty()) {
        JSValueRef l = ToJSValueRef(source->resource_line_offset, context);
        startingLineNumber = static_cast<int>(JSValueToNumber(ctx, l, 0));
    }
    if (!url) {
        url = "[undefined]";
    }

    JSString defaultError = "No source to parse";
    JSString errorMessage;
    int errorLine = 0;

    impl->m_script = 0;
    if (src.Length() > 0) {
        impl->m_script = JSCPrivate::JSScriptCreateFromString(iso->m_group, ctx, *url, startingLineNumber, *src, &errorMessage, &errorLine);
    } else {
        errorMessage = defaultError.Copy();
    }

    if (impl->m_script) {
        impl->m_script_string = src.Copy();
        impl->m_id.Reset(isolate, Integer::New(isolate, (int)reinterpret_cast<intptr_t>(impl->m_script)));
        impl->m_resource_line_offset.Reset(isolate, source->resource_line_offset);
        impl->m_resource_column_offset.Reset(isolate, source->resource_column_offset);
        impl->m_resource_is_shared_cross_origin = source->GetResourceOptions().IsSharedCrossOrigin();
        impl->m_resource_is_opaque = source->GetResourceOptions().IsOpaque();
        impl->m_is_wasm = source->GetResourceOptions().IsWasm();
        impl->m_is_module = source->GetResourceOptions().IsModule();
        
        return scope.Escape(CreateLocal<v8::UnboundScript>(&iso->ii, impl));
    }
    
    Local<v8::String> error = V82JSC::String::New(isolate, *errorMessage);
    char buffer[64];
    sprintf (buffer, " (at line %d)", errorLine);
    error = String::Concat(error, String::NewFromUtf8(isolate, buffer, NewStringType::kNormal).ToLocalChecked());
    
    String::Utf8Value str(error);
    printf ("Script error: %s\n", *str);
    
    LocalException exception(iso);
    JSValueRef *x = &exception;
    *x = ToJSValueRef(Exception::SyntaxError(error), context);
    
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
MaybeLocal<v8::Script> ScriptCompiler::Compile(Local<Context> context, Source* source,
                                           CompileOptions options, NoCacheReason no_cache_reason)
{
    Isolate *isolate = ToIsolate(ToContextImpl(context));
    Context::Scope context_scope(context);

    Local<String> backup;
    if (options == kConsumeParserCache || options == kConsumeCodeCache) {
        backup = source->source_string;
        Local<String> code = String::NewFromOneByte(isolate, source->GetCachedData()->data, NewStringType::kNormal).ToLocalChecked();
        source->source_string = code;
        if (options == kConsumeParserCache && !backup.IsEmpty() && !backup->Equals(context,code).FromJust()) {
            source->cached_data->rejected = true;
            source->source_string = backup;
        }
    }
    
    MaybeLocal<UnboundScript> unbound = ScriptCompiler::CompileUnboundScript(isolate, source);
    if (unbound.IsEmpty() && !backup.IsEmpty()) {
        source->source_string = backup;
        unbound = ScriptCompiler::CompileUnboundScript(isolate, source);
        source->cached_data->rejected = true;
    }
    if (!unbound.IsEmpty()) {
        if (options == kProduceParserCache || options == kProduceCodeCache) {
            if (!source->cached_data) {
                String::Utf8Value str(source->source_string);
                char *data = new char[strlen(*str) + 1];
                strcpy (data, *str);
                source->cached_data = new CachedData((uint8_t*)data, (int)strlen(*str));
            }
        }
        return unbound.ToLocalChecked()->BindToCurrentContext();
    }
    
    if (source->GetCachedData()) source->cached_data->rejected = true;

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
    return new V82JSCStreamingTask(source);
}

/**
 * Compiles a streamed script (bound to current context).
 *
 * This can only be called after the streaming has finished
 * (ScriptStreamingTask has been run). V8 doesn't construct the source string
 * during streaming, so the embedder needs to pass the full source here.
 */
MaybeLocal<v8::Script> ScriptCompiler::Compile(
                                           Local<Context> context, StreamedSource* source,
                                           Local<String> full_source_string, const ScriptOrigin& origin)
{
    ScriptCompiler::Source src(full_source_string, origin, const_cast<ScriptCompiler::CachedData*>(source->GetCachedData()));
    
    return Compile(context, &src);
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
    return (V8_MAJOR_VERSION<<24) + (V8_MINOR_VERSION<<16) + (V8_BUILD_NUMBER<<8) + (V8_PATCH_LEVEL);
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
      Local<Object> context_extensions[],
      CompileOptions options,
      NoCacheReason no_cache_reason)
{
    assert(0);
    return MaybeLocal<Function>();
}


/**
 * Creates and returns code cache for the specified unbound_script.
 * This will return nullptr if the script cannot be serialized. The
 * CachedData returned by this function should be owned by the caller.
 */
v8::ScriptCompiler::CachedData* ScriptCompiler::CreateCodeCache(Local<UnboundScript> unbound_script)
{
    printf("FIXME: ScriptCompiler::CreateCodeCache");
    return nullptr;
}
v8::ScriptCompiler::CachedData* ScriptCompiler::CreateCodeCacheForFunction(Local<Function> function)
{
    printf("FIXME: ScriptCompiler::CreateCodeCacheForFunction");
    return nullptr;
}

/**
 * The options that were passed by the embedder as HostDefinedOptions to
 * the ScriptOrigin.
 */
Local<PrimitiveArray> ScriptOrModule::GetHostDefinedOptions()
{
    assert(0);
    return Local<PrimitiveArray>();
}

