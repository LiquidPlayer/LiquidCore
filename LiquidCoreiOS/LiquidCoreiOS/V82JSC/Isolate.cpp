//
//  Isolate.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Isolate.h"
#include "V82JSC.h"
#include "JSHeapFinalizerPrivate.h"
#include "JSMarkingConstraintPrivate.h"
#include "JSWeakRefPrivate.h"

using namespace v8;
using V82JSC_HeapObject::HeapImpl;

#define H V82JSC_HeapObject

v8::Isolate *current = nullptr;

#define DEF(T,V,F) \
v8::internal::Object ** V;
struct Roots {
    STRONG_ROOT_LIST(DEF)
};
#define DECLARE_FIELDS(type,name,v) \
const intptr_t v8::internal::Isolate::name##_debug_offset_ = (reinterpret_cast<intptr_t>(&(reinterpret_cast<v8::internal::Isolate*>(16)->name##_)) - 16);
ISOLATE_INIT_LIST(DECLARE_FIELDS);

std::map<JSGlobalContextRef, IsolateImpl*> IsolateImpl::s_context_to_isolate_map;

static void MarkingConstraintCallback(JSMarkerRef marker, void *userData)
{
    IsolateImpl *impl = (IsolateImpl*)userData;
    impl->performIncrementalMarking(marker, impl->m_near_death);
    impl->m_pending_prologue = true;
}

static void HeapFinalizerCallback(JSContextGroupRef grp, void *userData)
{
    IsolateImpl *impl = (IsolateImpl*)userData;
    impl->m_pending_epilogue = true;
}

/**
 * Creates a new isolate.  Does not change the currently entered
 * isolate.
 *
 * When an isolate is no longer used its resources should be freed
 * by calling Dispose().  Using the delete operator is not allowed.
 *
 * V8::Initialize() must have run prior to this.
 */
Isolate * Isolate::New(Isolate::CreateParams const&params)
{
    IsolateImpl * impl = (IsolateImpl *) malloc(sizeof (IsolateImpl));
    memset(impl, 0, sizeof(IsolateImpl));
    Isolate * isolate = V82JSC::ToIsolate(impl);

    reinterpret_cast<internal::Isolate*>(isolate)->Init((v8::internal::Deserializer *)&params);

    HeapImpl* heap = static_cast<HeapImpl*>(impl->ii.heap());
    heap->m_heap_top = nullptr;
    heap->m_allocated = 0;
    
    impl->m_global_symbols = std::map<std::string, JSValueRef>();
    impl->m_private_symbols = std::map<std::string, JSValueRef>();
    impl->m_context_stack = std::stack<Copyable(v8::Context)>();
    impl->m_scope_stack = std::stack<HandleScope*>();
    impl->m_global_contexts = std::map<JSGlobalContextRef, Copyable(Context)>();
    impl->m_exec_maps = std::map<JSGlobalContextRef, std::map<const char *, JSObjectRef>>();
    impl->m_message_listeners = std::vector<internal::MessageListener>();
    impl->m_running_scripts = std::stack<Local<v8::Script>>();
    impl->m_gc_prologue_callbacks = std::vector<IsolateImpl::GCCallbackStruct>();
    impl->m_gc_epilogue_callbacks = std::vector<IsolateImpl::GCCallbackStruct>();
    impl->m_near_death = std::map<void*, JSObjectRef>();
    impl->m_second_pass_callbacks = std::vector<internal::SecondPassCallback>();
    impl->m_external_strings = std::map<JSValueRef, Copyable(v8::WeakExternalString)>();
    
    HandleScope scope(isolate);
    
    impl->m_params = params;
    
    impl->m_group = JSContextGroupCreate();
    
    JSContextGroupAddMarkingConstraint(impl->m_group, MarkingConstraintCallback, impl);
    JSContextGroupAddHeapFinalizer(impl->m_group, HeapFinalizerCallback, impl);
    
    Roots* roots = reinterpret_cast<Roots *>(impl->ii.heap()->roots_array_start());
    
    // Create Map Objects
    impl->m_context_map = H::Map<H::Context>::New(impl, internal::CONTEXT_EXTENSION_TYPE);
    impl->m_global_context_map = H::Map<H::GlobalContext>::New(impl, internal::CONTEXT_EXTENSION_TYPE);
    auto map_undefined = H::Map<H::Value>::New(impl, internal::ODDBALL_TYPE, internal::Internals::kUndefinedOddballKind);
    auto map_the_hole = H::Map<H::Value>::New(impl, internal::ODDBALL_TYPE, internal::Internals::kUndefinedOddballKind);
    auto map_null = H::Map<H::Value>::New(impl, internal::ODDBALL_TYPE, internal::Internals::kNullOddballKind);
    auto map_true = H::Map<H::Value>::New(impl, internal::JS_VALUE_TYPE);
    auto map_false = H::Map<H::Value>::New(impl, internal::JS_VALUE_TYPE);
    auto map_empty_string = H::Map<H::Value>::New(impl, internal::INTERNALIZED_STRING_TYPE);
    impl->m_tracked_object_map = H::Map<H::TrackedObject>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_array_buffer_map = H::Map<H::Value>::New(impl, internal::JS_ARRAY_BUFFER_TYPE);
    impl->m_fixed_array_map = H::Map<H::FixedArray>::New(impl, internal::FIXED_ARRAY_TYPE);
    impl->m_one_byte_string_map = H::Map<H::String>::New(impl, internal::ONE_BYTE_INTERNALIZED_STRING_TYPE);
    impl->m_string_map = H::Map<H::String>::New(impl, internal::INTERNALIZED_STRING_TYPE);
    impl->m_external_one_byte_string_map = H::Map<H::String>::New(impl, internal::EXTERNAL_ONE_BYTE_STRING_TYPE);
    impl->m_external_string_map = H::Map<H::String>::New(impl, internal::EXTERNAL_STRING_TYPE);
    impl->m_weak_external_one_byte_string_map = H::Map<H::WeakExternalString>::New(impl, internal::EXTERNAL_ONE_BYTE_STRING_TYPE);
    impl->m_weak_external_string_map = H::Map<H::WeakExternalString>::New(impl, internal::EXTERNAL_STRING_TYPE);
    impl->m_internalized_string_map = H::Map<H::String>::New(impl, internal::INTERNALIZED_STRING_TYPE);
    impl->m_value_map = H::Map<H::Value>::New(impl, internal::JS_VALUE_TYPE);
    impl->m_number_map = H::Map<H::Value>::New(impl, internal::HEAP_NUMBER_TYPE);
    impl->m_symbol_map = H::Map<H::Value>::New(impl, internal::SYMBOL_TYPE);
    impl->m_signature_map = H::Map<H::Signature>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_function_template_map = H::Map<H::FunctionTemplate>::New(impl, internal::FUNCTION_TEMPLATE_INFO_TYPE);
    impl->m_object_template_map = H::Map<H::ObjectTemplate>::New(impl, internal::OBJECT_TEMPLATE_INFO_TYPE);
    impl->m_property_map = H::Map<H::Prop>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_property_accessor_map = H::Map<H::PropAccessor>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_intrinsic_property_map = H::Map<H::IntrinsicProp>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_accessor_map = H::Map<H::Accessor>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_object_accessor_map = H::Map<H::ObjAccessor>::New(impl, internal::JS_SPECIAL_API_OBJECT_TYPE);
    impl->m_unbound_script_map = H::Map<H::UnboundScript>::New(impl, internal::SHARED_FUNCTION_INFO_TYPE);
    impl->m_script_map = H::Map<H::Script>::New(impl, internal::SCRIPT_TYPE);
    impl->m_weak_value_map = H::Map<H::WeakValue>::New(impl, internal::WEAK_CELL_TYPE);
    impl->m_stack_frame_map = H::Map<H::StackFrame>::New(impl, internal::STACK_FRAME_INFO_TYPE);
    impl->m_stack_trace_map = H::Map<H::StackTrace>::New(impl, internal::STACK_FRAME_INFO_TYPE);
    impl->m_message_map = H::Map<H::Message>::New(impl, internal::JS_MESSAGE_OBJECT_TYPE);

    roots->block_context_map = reinterpret_cast<internal::Object**>(H::ToV8Map(impl->m_context_map));
    roots->native_context_map = reinterpret_cast<internal::Object**>(H::ToV8Map(impl->m_global_context_map));
    roots->undefined_value = reinterpret_cast<internal::Object**> (H::ToV8Map(map_undefined));
    roots->the_hole_value = reinterpret_cast<internal::Object**> (H::ToV8Map(map_the_hole));
    roots->null_value = reinterpret_cast<internal::Object**> (H::ToV8Map(map_null));
    roots->true_value = reinterpret_cast<internal::Object**> (H::ToV8Map(map_true));
    roots->false_value = reinterpret_cast<internal::Object**> (H::ToV8Map(map_false));
    roots->empty_string = reinterpret_cast<internal::Object**> (H::ToV8Map(map_empty_string));

    Local<Context> nullContext = Context::New(isolate);
    impl->m_nullContext.Reset(isolate, nullContext);
    
    Context::Scope context_scope(nullContext);

    JSStringRef empty_string = JSStringCreateWithUTF8CString("");
    impl->m_empty_string = JSValueMakeString(V82JSC::ToContextRef(nullContext), empty_string);
    JSValueProtect(V82JSC::ToContextRef(nullContext), impl->m_empty_string);
    JSStringRelease(empty_string);

    impl->m_negative_zero = V82JSC::exec(V82JSC::ToContextRef(nullContext), "return -0", 0, 0);
    JSValueProtect(V82JSC::ToContextRef(nullContext), impl->m_negative_zero);

    impl->m_private_symbol = V82JSC::exec(V82JSC::ToContextRef(nullContext), "return Symbol()", 0, 0);
    JSValueProtect(V82JSC::ToContextRef(nullContext), impl->m_private_symbol);
    
    impl->m_proxy_revocables = (JSObjectRef) V82JSC::exec(V82JSC::ToContextRef(nullContext), "return new WeakMap()", 0, 0);
    JSValueProtect(V82JSC::ToContextRef(nullContext), impl->m_proxy_revocables);

    impl->ii.thread_local_top_.isolate_ = &impl->ii;
    impl->ii.thread_local_top_.scheduled_exception_ = reinterpret_cast<internal::Object*>(roots->the_hole_value);
    impl->ii.thread_local_top_.pending_exception_ = reinterpret_cast<internal::Object*>(roots->the_hole_value);
    
    return reinterpret_cast<v8::Isolate*>(isolate);
}

void IsolateImpl::TriggerGCPrologue()
{
    if (!m_pending_prologue) return;
    m_pending_prologue = false;
    
    for (auto i=m_gc_prologue_callbacks.begin(); i != m_gc_prologue_callbacks.end(); ++i) {
        if ((*i).m_callback)
            (*i).m_callback(V82JSC::ToIsolate(this), kGCTypeIncrementalMarking, kNoGCCallbackFlags);
        else
            (*i).m_callback_with_data(V82JSC::ToIsolate(this), kGCTypeIncrementalMarking, kNoGCCallbackFlags, (*i).m_data);
    }
}

void IsolateImpl::TriggerGCFirstPassPhantomCallbacks()
{
    // Call weak first-pass callbacks
    for (auto i=m_near_death.begin(); i!=m_near_death.end(); ++i) {
        weakObjectNearDeath(reinterpret_cast<internal::Object**>(i->first),
                            m_second_pass_callbacks,
                            i->second);
    }
    m_near_death.clear();
}

void IsolateImpl::TriggerGCEpilogue()
{
    if (!m_pending_epilogue) return;
    m_pending_epilogue = false;

    for (auto i=m_gc_epilogue_callbacks.begin(); i != m_gc_epilogue_callbacks.end(); ++i) {
        if ((*i).m_callback)
            (*i).m_callback(V82JSC::ToIsolate(this), kGCTypeScavenge, kNoGCCallbackFlags);
        else
            (*i).m_callback_with_data(V82JSC::ToIsolate(this), kGCTypeScavenge, kNoGCCallbackFlags, (*i).m_data);
    }
}

void IsolateImpl::CollectExternalStrings()
{
    v8::HandleScope scope(V82JSC::ToIsolate(this));
    for (auto i=m_external_strings.begin(); i!=m_external_strings.end(); ) {
        WeakExternalStringImpl *ext = V82JSC::ToImpl<WeakExternalStringImpl>(i->second.Get(V82JSC::ToIsolate(this)));
        if (JSWeakGetObject(ext->m_weakRef) == 0) {
            i = m_external_strings.erase(i);
        } else {
            ++i;
        }
    }
}

JSGlobalContextRef H::HeapObject::GetNullContext()
{
    IsolateImpl* iso = GetIsolate();
    HandleScope(V82JSC::ToIsolate(iso));
    Local<v8::Context> ctx = iso->m_nullContext.Get(V82JSC::ToIsolate(iso));
    return JSContextGetGlobalContext(V82JSC::ToContextRef(ctx));
}

JSContextGroupRef H::HeapObject::GetContextGroup()
{
    IsolateImpl* iso = GetIsolate();
    return iso->m_group;
}

bool internal::Isolate::Init(v8::internal::Deserializer *des)
{
    v8::Isolate::CreateParams& create = *(v8::Isolate::CreateParams*)des;
    internal::Heap* h = heap();
    h->isolate_ = this;
    h->max_old_generation_size_ = create.constraints.max_old_space_size() * 1024 * 1024;
    h->initial_max_old_generation_size_ = h->max_old_generation_size_;
    
    global_handles_ = new internal::GlobalHandles(this);
    return true;
}

/**
 * Returns the entered isolate for the current thread or NULL in
 * case there is no current isolate.
 *
 * This method must not be invoked before V8::Initialize() was invoked.
 */
Isolate* Isolate::GetCurrent()
{
    return current;
}

/**
 * Custom callback used by embedders to help V8 determine if it should abort
 * when it throws and no internal handler is predicted to catch the
 * exception. If --abort-on-uncaught-exception is used on the command line,
 * then V8 will abort if either:
 * - no custom callback is set.
 * - the custom callback set returns true.
 * Otherwise, the custom callback will not be called and V8 will not abort.
 */
void Isolate::SetAbortOnUncaughtExceptionCallback(AbortOnUncaughtExceptionCallback callback)
{
    assert(0);
}

/**
 * This is an unfinished experimental feature, and is only exposed
 * here for internal testing purposes. DO NOT USE.
 *
 * This specifies the callback called by the upcoming dynamic
 * import() language feature to load modules.
 */
void Isolate::SetHostImportModuleDynamicallyCallback(HostImportModuleDynamicallyCallback callback)
{
    assert(0);
}

/**
 * Optional notification that the system is running low on memory.
 * V8 uses these notifications to guide heuristics.
 * It is allowed to call this function from another thread while
 * the isolate is executing long running JavaScript code.
 */
void Isolate::MemoryPressureNotification(MemoryPressureLevel level)
{
    assert(0);
}

/**
 * Methods below this point require holding a lock (using Locker) in
 * a multi-threaded environment.
 */

/**
 * Sets this isolate as the entered one for the current thread.
 * Saves the previously entered one (if any), so that it can be
 * restored when exiting.  Re-entering an isolate is allowed.
 */
void Isolate::Enter()
{
    current = this;
}

/**
 * Exits this isolate by restoring the previously entered one in the
 * current thread.  The isolate may still stay the same, if it was
 * entered more than once.
 *
 * Requires: this == Isolate::GetCurrent().
 */
void Isolate::Exit()
{
    current = nullptr;
}

void IsolateImpl::EnterContext(Local<v8::Context> ctx)
{
    Copyable(v8::Context) persist(V82JSC::ToIsolate(this), ctx);
    m_context_stack.push(persist);
    persist.Reset();
}

void IsolateImpl::ExitContext(Local<v8::Context> ctx)
{
    assert(m_context_stack.size());
    JSContextRef top = V82JSC::ToContextRef(m_context_stack.top().Get(V82JSC::ToIsolate(this)));
    JSContextRef newctx = V82JSC::ToContextRef(ctx);
    assert(top == newctx);

    m_context_stack.top().Reset();
    m_context_stack.pop();
}

/**
 * Disposes the isolate.  The isolate must not be entered by any
 * thread to be disposable.
 */

JS_EXPORT void JSContextGroupRemoveMarkingConstraint(JSContextGroupRef, JSMarkingConstraint, void *userData);

void Isolate::Dispose()
{
    IsolateImpl *isolate = (IsolateImpl *)this;
    if (current == this) {
        if (isolate->m_fatal_error_callback) {
            isolate->m_fatal_error_callback("Isolate::Dispose()", "Attempting to dispose an entered isolate.");
        } else {
            FATAL("Attempting to dispose an entered isolate.");
        }
        return;
    }

    // Hmm.  There is no JSContextGroupRemoveMarkingConstraint() equivalent
    JSContextGroupRemoveHeapFinalizer(isolate->m_group, HeapFinalizerCallback, isolate);
    
    {
        HandleScope scope(V82JSC::ToIsolate(isolate));
        while (!isolate->m_external_strings.empty()) {
            Local<WeakExternalString> wes = isolate->m_external_strings.begin()->second.Get(V82JSC::ToIsolate(isolate));
            WeakExternalStringImpl *impl = V82JSC::ToImpl<WeakExternalStringImpl>(wes);
            if (impl->m_resource) {
                intptr_t addr = reinterpret_cast<intptr_t>(impl) + v8::internal::ExternalString::kResourceOffset;
                * reinterpret_cast<v8::String::ExternalStringResourceBase**>(addr) = impl->m_resource;
                isolate->ii.heap()->FinalizeExternalString(reinterpret_cast<v8::internal::String*>(ToHeapPointer(impl)));
            }
            isolate->m_external_strings.erase(isolate->m_external_strings.begin());
        }
    }

    JSContextGroupRelease(isolate->m_group);
    for (auto i=isolate->m_global_contexts.begin(); i != isolate->m_global_contexts.end(); i++) {
        IsolateImpl::s_context_to_isolate_map.erase(i->first);
    }
    isolate->m_global_contexts.clear();
    isolate->m_exec_maps.clear();
    while (!isolate->m_context_stack.empty()) isolate->m_context_stack.pop();
    isolate->m_nullContext.Reset();
    
    while (isolate->m_handlers) {
        TryCatchCopy *tcc = reinterpret_cast<TryCatchCopy*>(isolate->m_handlers);
        isolate->m_handlers = tcc->next_;
        delete(tcc);
    }
    
    isolate->m_global_symbols.clear();
    isolate->m_private_symbols.clear();
    isolate->m_message_listeners.clear();
    while (!isolate->m_running_scripts.empty()) isolate->m_running_scripts.pop();
    isolate->m_gc_prologue_callbacks.clear();
    isolate->m_gc_epilogue_callbacks.clear();
    isolate->m_second_pass_callbacks.clear();
    
    // Finally, blitz the global handles and the heap
    isolate->ii.global_handles()->TearDown();
    V82JSC_HeapObject::HeapAllocator::TearDown(isolate);

    // And now we the isolate is done
    free(isolate);
}

void IsolateImpl::CollectGarbage()
{
    if (m_callback_depth != 0) {
        m_pending_garbage_collection = true;
    } else {
        H::HeapAllocator::CollectGarbage(this);
    }
}

/**
 * Dumps activated low-level V8 internal stats. This can be used instead
 * of performing a full isolate disposal.
 */
void Isolate::DumpAndResetStats()
{
    assert(0);
}

/**
 * Discards all V8 thread-specific data for the Isolate. Should be used
 * if a thread is terminating and it has used an Isolate that will outlive
 * the thread -- all thread-specific data for an Isolate is discarded when
 * an Isolate is disposed so this call is pointless if an Isolate is about
 * to be Disposed.
 */
void Isolate::DiscardThreadSpecificMetadata()
{
    assert(0);
}

/**
 * Get statistics about the heap memory usage.
 */
void Isolate::GetHeapStatistics(HeapStatistics* heap_statistics)
{
    internal::Heap *heap = reinterpret_cast<internal::Isolate*>(this)->heap();
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(heap);
    H::HeapAllocator *chunk = static_cast<H::HeapAllocator*>(heapimpl->m_heap_top);

    heap_statistics->total_heap_size_ = 0;
    for ( ; chunk != nullptr; chunk=static_cast<H::HeapAllocator*>(chunk->next_chunk())) {
        heap_statistics->total_heap_size_ += HEAP_ALIGNMENT;
    }
    
    heap_statistics->used_heap_size_ = heapimpl->m_allocated;
}

size_t internal::Heap::SizeOfObjects()
{
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(this);
    return heapimpl->m_allocated;
}

/**
 * Returns the number of spaces in the heap.
 */
size_t Isolate::NumberOfHeapSpaces()
{
    assert(0);
    return 0;
}

/**
 * Get the memory usage of a space in the heap.
 *
 * \param space_statistics The HeapSpaceStatistics object to fill in
 *   statistics.
 * \param index The index of the space to get statistics from, which ranges
 *   from 0 to NumberOfHeapSpaces() - 1.
 * \returns true on success.
 */
bool Isolate::GetHeapSpaceStatistics(HeapSpaceStatistics* space_statistics,
                            size_t index)
{
    assert(0);
    return false;
}

/**
 * Returns the number of types of objects tracked in the heap at GC.
 */
size_t Isolate::NumberOfTrackedHeapObjectTypes()
{
    assert(0);
    return 0;
}

/**
 * Get statistics about objects in the heap.
 *
 * \param object_statistics The HeapObjectStatistics object to fill in
 *   statistics of objects of given type, which were live in the previous GC.
 * \param type_index The index of the type of object to fill details about,
 *   which ranges from 0 to NumberOfTrackedHeapObjectTypes() - 1.
 * \returns true on success.
 */
bool Isolate::GetHeapObjectStatisticsAtLastGC(HeapObjectStatistics* object_statistics,
                                     size_t type_index)
{
    assert(0);
    return false;
}

/**
 * Get statistics about code and its metadata in the heap.
 *
 * \param object_statistics The HeapCodeStatistics object to fill in
 *   statistics of code, bytecode and their metadata.
 * \returns true on success.
 */
bool Isolate::GetHeapCodeAndMetadataStatistics(HeapCodeStatistics* object_statistics)
{
    assert(0);
    return false;
}

/**
 * Get a call stack sample from the isolate.
 * \param state Execution state.
 * \param frames Caller allocated buffer to store stack frames.
 * \param frames_limit Maximum number of frames to capture. The buffer must
 *                     be large enough to hold the number of frames.
 * \param sample_info The sample info is filled up by the function
 *                    provides number of actual captured stack frames and
 *                    the current VM state.
 * \note GetStackSample should only be called when the JS thread is paused or
 *       interrupted. Otherwise the behavior is undefined.
 */
void Isolate::GetStackSample(const RegisterState& state, void** frames,
                    size_t frames_limit, SampleInfo* sample_info)
{
    assert(0);
}

/**
 * Returns the number of phantom handles without callbacks that were reset
 * by the garbage collector since the last call to this function.
 */
size_t Isolate::NumberOfPhantomHandleResetsSinceLastCall()
{
    assert(0);
    return 0;
}

/**
 * Returns heap profiler for this isolate. Will return NULL until the isolate
 * is initialized.
 */
HeapProfiler* Isolate::GetHeapProfiler()
{
    assert(0);
    return nullptr;
}

/** Returns true if this isolate has a current context. */
bool Isolate::InContext()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(this);
    return impl->m_context_stack.size() != 0;
}

/**
 * Returns the context of the currently running JavaScript, or the context
 * on the top of the stack if no JavaScript is running.
 */
Local<Context> Isolate::GetCurrentContext()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(this);
    if (!impl->m_context_stack.size()) {
        return Local<Context>();
    }
    
    return Local<Context>::New(this, impl->m_context_stack.top());
}

/** Returns the last context entered through V8's C++ API. */
Local<Context> Isolate::GetEnteredContext()
{
    assert(0);
    return Local<Context>();
}

/**
 * Returns either the last context entered through V8's C++ API, or the
 * context of the currently running microtask while processing microtasks.
 * If a context is entered while executing a microtask, that context is
 * returned.
 */
Local<Context> Isolate::GetEnteredOrMicrotaskContext()
{
    assert(0);
    return Local<Context>();
}

/**
 * Returns the Context that corresponds to the Incumbent realm in HTML spec.
 * https://html.spec.whatwg.org/multipage/webappapis.html#incumbent
 */
Local<Context> Isolate::GetIncumbentContext()
{
    assert(0);
    return Local<Context>();
}

/**
 * Schedules an exception to be thrown when returning to JavaScript.  When an
 * exception has been scheduled it is illegal to invoke any JavaScript
 * operation; the caller must return immediately and only after the exception
 * has been handled does it become legal to invoke JavaScript operations.
 */
Local<Value> Isolate::ThrowException(Local<Value> exception)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    if (exception.IsEmpty()) {
        impl->ii.thread_local_top()->scheduled_exception_ = impl->ii.heap()->root(v8::internal::Heap::RootListIndex::kUndefinedValueRootIndex);
    } else {
        impl->ii.thread_local_top()->scheduled_exception_ = * reinterpret_cast<internal::Object**>(*exception);
    }

    return exception;
}

/**
 * Enables the host application to receive a notification before a
 * garbage collection. Allocations are allowed in the callback function,
 * but the callback is not re-entrant: if the allocation inside it will
 * trigger the garbage collection, the callback won't be called again.
 * It is possible to specify the GCType filter for your callback. But it is
 * not possible to register the same callback function two times with
 * different GCType filters.
 */
void Isolate::AddGCPrologueCallback(GCCallbackWithData callback, void* data,
                           GCType gc_type_filter)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    struct IsolateImpl::GCCallbackStruct cb;
    cb.m_callback = nullptr;
    cb.m_callback_with_data = callback;
    cb.m_data = data;
    cb.m_gc_type_filter = gc_type_filter;
    
    impl->m_gc_prologue_callbacks.push_back(cb);
}
void Isolate::AddGCPrologueCallback(GCCallback callback, GCType gc_type_filter)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    struct IsolateImpl::GCCallbackStruct cb;
    cb.m_callback = callback;
    cb.m_callback_with_data = nullptr;
    cb.m_data = nullptr;
    cb.m_gc_type_filter = gc_type_filter;
    
    impl->m_gc_prologue_callbacks.push_back(cb);
}

/**
 * This function removes callback which was installed by
 * AddGCPrologueCallback function.
 */
void Isolate::RemoveGCPrologueCallback(GCCallbackWithData callback, void* data)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    for (auto i=impl->m_gc_prologue_callbacks.begin(); i!=impl->m_gc_prologue_callbacks.end(); ) {
        if ((*i).m_callback_with_data == callback && (*i).m_data == data)
            impl->m_gc_prologue_callbacks.erase(i);
        else
            ++i;
    }
}
void Isolate::RemoveGCPrologueCallback(GCCallback callback)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    for (auto i=impl->m_gc_prologue_callbacks.begin(); i!=impl->m_gc_prologue_callbacks.end(); ) {
        if ((*i).m_callback == callback)
            impl->m_gc_prologue_callbacks.erase(i);
        else
            ++i;
    }
}

/**
 * Sets the embedder heap tracer for the isolate.
 */
void Isolate::SetEmbedderHeapTracer(EmbedderHeapTracer* tracer)
{
    assert(0);
}

/**
 * Enables the host application to receive a notification after a
 * garbage collection. Allocations are allowed in the callback function,
 * but the callback is not re-entrant: if the allocation inside it will
 * trigger the garbage collection, the callback won't be called again.
 * It is possible to specify the GCType filter for your callback. But it is
 * not possible to register the same callback function two times with
 * different GCType filters.
 */
void Isolate::AddGCEpilogueCallback(GCCallbackWithData callback, void* data, GCType gc_type_filter)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    struct IsolateImpl::GCCallbackStruct cb;
    cb.m_callback = nullptr;
    cb.m_callback_with_data = callback;
    cb.m_data = data;
    cb.m_gc_type_filter = gc_type_filter;
    
    impl->m_gc_epilogue_callbacks.push_back(cb);
}
void Isolate::AddGCEpilogueCallback(GCCallback callback, GCType gc_type_filter)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    struct IsolateImpl::GCCallbackStruct cb;
    cb.m_callback = callback;
    cb.m_callback_with_data = nullptr;
    cb.m_data = nullptr;
    cb.m_gc_type_filter = gc_type_filter;
    
    impl->m_gc_epilogue_callbacks.push_back(cb);
}

/**
 * This function removes callback which was installed by
 * AddGCEpilogueCallback function.
 */
void Isolate::RemoveGCEpilogueCallback(GCCallbackWithData callback, void* data)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    for (auto i=impl->m_gc_epilogue_callbacks.begin(); i!=impl->m_gc_epilogue_callbacks.end(); ) {
        if ((*i).m_callback_with_data == callback && (*i).m_data == data)
            impl->m_gc_epilogue_callbacks.erase(i);
        else
            ++i;
    }
}
void Isolate::RemoveGCEpilogueCallback(GCCallback callback)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    for (auto i=impl->m_gc_epilogue_callbacks.begin(); i!=impl->m_gc_epilogue_callbacks.end(); ) {
        if ((*i).m_callback == callback)
            impl->m_gc_epilogue_callbacks.erase(i);
        else
            ++i;
    }
}

/**
 * Set the callback that tells V8 how much memory is currently allocated
 * externally of the V8 heap. Ideally this memory is somehow connected to V8
 * objects and may get freed-up when the corresponding V8 objects get
 * collected by a V8 garbage collection.
 */
void Isolate::SetGetExternallyAllocatedMemoryInBytesCallback(
                                                    GetExternallyAllocatedMemoryInBytesCallback callback)
{
    assert(0);
}

/**
 * Forcefully terminate the current thread of JavaScript execution
 * in the given isolate.
 *
 * This method can be used by any thread even if that thread has not
 * acquired the V8 lock with a Locker object.
 */
void Isolate::TerminateExecution()
{
    assert(0);
}

/**
 * Is V8 terminating JavaScript execution.
 *
 * Returns true if JavaScript execution is currently terminating
 * because of a call to TerminateExecution.  In that case there are
 * still JavaScript frames on the stack and the termination
 * exception is still active.
 */
bool Isolate::IsExecutionTerminating()
{
    assert(0);
    return false;
}

/**
 * Resume execution capability in the given isolate, whose execution
 * was previously forcefully terminated using TerminateExecution().
 *
 * When execution is forcefully terminated using TerminateExecution(),
 * the isolate can not resume execution until all JavaScript frames
 * have propagated the uncatchable exception which is generated.  This
 * method allows the program embedding the engine to handle the
 * termination event and resume execution capability, even if
 * JavaScript frames remain on the stack.
 *
 * This method can be used by any thread even if that thread has not
 * acquired the V8 lock with a Locker object.
 */
void Isolate::CancelTerminateExecution()
{
    assert(0);
}

/**
 * Request V8 to interrupt long running JavaScript code and invoke
 * the given |callback| passing the given |data| to it. After |callback|
 * returns control will be returned to the JavaScript code.
 * There may be a number of interrupt requests in flight.
 * Can be called from another thread without acquiring a |Locker|.
 * Registered |callback| must not reenter interrupted Isolate.
 */
void Isolate::RequestInterrupt(InterruptCallback callback, void* data)
{
    assert(0);
}

#ifdef __cplusplus
extern "C" {
#endif
    void JSSynchronousGarbageCollectForDebugging(JSContextRef);
#ifdef __cplusplus
}
#endif

/**
 * Request garbage collection in this Isolate. It is only valid to call this
 * function if --expose_gc was specified.
 *
 * This should only be used for testing purposes and not to enforce a garbage
 * collection schedule. It has strong negative impact on the garbage
 * collection performance. Use IdleNotificationDeadline() or
 * LowMemoryNotification() instead to influence the garbage collection
 * schedule.
 */
void Isolate::RequestGarbageCollectionForTesting(GarbageCollectionType type)
{
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(this);
    if (iso->m_in_gc++) return;
    
    // First pass, clear anything on the V82JSC side that is not in use
    iso->CollectGarbage();
    
    // Next, trigger garbage collection in JSC
    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
    }
    
    iso->TriggerGCPrologue();
    iso->TriggerGCFirstPassPhantomCallbacks();
    
    // Next, trigger garbage collection in JSC (do it twice -- sometimes the first doesn't finish the job)
    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
    }
    
    iso->TriggerGCFirstPassPhantomCallbacks();
    iso->CollectExternalStrings();
    
    // Second pass, clear V82JSC garbage again in case any weak references were cleared
    iso->CollectGarbage();
    iso->TriggerGCEpilogue();
    
    iso->m_in_gc = 0;
}

/**
 * Set the callback to invoke for logging event.
 */
void Isolate::SetEventLogger(LogEventCallback that)
{
    assert(0);
}

/**
 * Adds a callback to notify the host application right before a script
 * is about to run. If a script re-enters the runtime during executing, the
 * BeforeCallEnteredCallback is invoked for each re-entrance.
 * Executing scripts inside the callback will re-trigger the callback.
 */
void Isolate::AddBeforeCallEnteredCallback(BeforeCallEnteredCallback callback)
{
    assert(0);
}

/**
 * Removes callback that was installed by AddBeforeCallEnteredCallback.
 */
void Isolate::RemoveBeforeCallEnteredCallback(BeforeCallEnteredCallback callback)
{
    assert(0);
}

/**
 * Adds a callback to notify the host application when a script finished
 * running.  If a script re-enters the runtime during executing, the
 * CallCompletedCallback is only invoked when the outer-most script
 * execution ends.  Executing scripts inside the callback do not trigger
 * further callbacks.
 */
void Isolate::AddCallCompletedCallback(CallCompletedCallback callback)
{
    assert(0);
}

/**
 * Removes callback that was installed by AddCallCompletedCallback.
 */
void Isolate::RemoveCallCompletedCallback(CallCompletedCallback callback)
{
    assert(0);
}

/**
 * Experimental: Set the PromiseHook callback for various promise
 * lifecycle events.
 */
void Isolate::SetPromiseHook(PromiseHook hook)
{
    assert(0);
}

/**
 * Set callback to notify about promise reject with no handler, or
 * revocation of such a previous notification once the handler is added.
 */
void Isolate::SetPromiseRejectCallback(PromiseRejectCallback callback)
{
    assert(0);
}

/**
 * Experimental: Runs the Microtask Work Queue until empty
 * Any exceptions thrown by microtask callbacks are swallowed.
 */
void Isolate::RunMicrotasks()
{
    assert(0);
}

/**
 * Experimental: Enqueues the callback to the Microtask Work Queue
 */
void Isolate::EnqueueMicrotask(Local<Function> microtask)
{
    assert(0);
}

/**
 * Experimental: Enqueues the callback to the Microtask Work Queue
 */
void Isolate::EnqueueMicrotask(MicrotaskCallback microtask, void* data)
{
    assert(0);
}

/**
 * Experimental: Controls how Microtasks are invoked. See MicrotasksPolicy
 * for details.
 */
void Isolate::SetMicrotasksPolicy(MicrotasksPolicy policy)
{
    assert(0);
}

/**
 * Experimental: Returns the policy controlling how Microtasks are invoked.
 */
MicrotasksPolicy Isolate::GetMicrotasksPolicy() const
{
    assert(0);
    return MicrotasksPolicy();
}

/**
 * Experimental: adds a callback to notify the host application after
 * microtasks were run. The callback is triggered by explicit RunMicrotasks
 * call or automatic microtasks execution (see SetAutorunMicrotasks).
 *
 * Callback will trigger even if microtasks were attempted to run,
 * but the microtasks queue was empty and no single microtask was actually
 * executed.
 *
 * Executing scriptsinside the callback will not re-trigger microtasks and
 * the callback.
 */
void Isolate::AddMicrotasksCompletedCallback(MicrotasksCompletedCallback callback)
{
    assert(0);
}

/**
 * Removes callback that was installed by AddMicrotasksCompletedCallback.
 */
void Isolate::RemoveMicrotasksCompletedCallback(MicrotasksCompletedCallback callback)
{
    assert(0);
}

/**
 * Sets a callback for counting the number of times a feature of V8 is used.
 */
void Isolate::SetUseCounterCallback(UseCounterCallback callback)
{
    assert(0);
}

/**
 * Enables the host application to provide a mechanism for recording
 * statistics counters.
 */
void Isolate::SetCounterFunction(CounterLookupCallback)
{
    assert(0);
}

/**
 * Enables the host application to provide a mechanism for recording
 * histograms. The CreateHistogram function returns a
 * histogram which will later be passed to the AddHistogramSample
 * function.
 */
void Isolate::SetCreateHistogramFunction(CreateHistogramCallback)
{
    assert(0);
}
void Isolate::SetAddHistogramSampleFunction(AddHistogramSampleCallback)
{
    assert(0);
}

/**
 * Optional notification that the embedder is idle.
 * V8 uses the notification to perform garbage collection.
 * This call can be used repeatedly if the embedder remains idle.
 * Returns true if the embedder should stop calling IdleNotificationDeadline
 * until real work has been done.  This indicates that V8 has done
 * as much cleanup as it will be able to do.
 *
 * The deadline_in_seconds argument specifies the deadline V8 has to finish
 * garbage collection work. deadline_in_seconds is compared with
 * MonotonicallyIncreasingTime() and should be based on the same timebase as
 * that function. There is no guarantee that the actual work will be done
 * within the time limit.
 */
bool Isolate::IdleNotificationDeadline(double deadline_in_seconds)
{
    assert(0);
    return false;
}

/**
 * Optional notification that the system is running low on memory.
 * V8 uses these notifications to attempt to free memory.
 */
void Isolate::LowMemoryNotification()
{
    assert(0);
}

/**
 * Optional notification that a context has been disposed. V8 uses
 * these notifications to guide the GC heuristic. Returns the number
 * of context disposals - including this one - since the last time
 * V8 had a chance to clean up.
 *
 * The optional parameter |dependant_context| specifies whether the disposed
 * context was depending on state from other contexts or not.
 */
int Isolate::ContextDisposedNotification(bool dependant_context)
{
    return 0;
}

/**
 * Optional notification that the isolate switched to the foreground.
 * V8 uses these notifications to guide heuristics.
 */
void Isolate::IsolateInForegroundNotification()
{
    assert(0);
}

/**
 * Optional notification that the isolate switched to the background.
 * V8 uses these notifications to guide heuristics.
 */
void Isolate::IsolateInBackgroundNotification()
{
    assert(0);
}

/**
 * Optional notification to tell V8 the current performance requirements
 * of the embedder based on RAIL.
 * V8 uses these notifications to guide heuristics.
 * This is an unfinished experimental feature. Semantics and implementation
 * may change frequently.
 */
void Isolate::SetRAILMode(RAILMode rail_mode)
{
    assert(0);
}

/**
 * Optional notification to tell V8 the current isolate is used for debugging
 * and requires higher heap limit.
 */
void Isolate::IncreaseHeapLimitForDebugging()
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    HeapImpl* heap = static_cast<HeapImpl*>(impl->ii.heap());
    heap->IncreaseHeapLimitForDebugging();
}

/**
 * Restores the original heap limit after IncreaseHeapLimitForDebugging().
 */
void Isolate::RestoreOriginalHeapLimit()
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    HeapImpl* heap = static_cast<HeapImpl*>(impl->ii.heap());
    heap->RestoreOriginalHeapLimit();
}

/**
 * Returns true if the heap limit was increased for debugging and the
 * original heap limit was not restored yet.
 */
bool Isolate::IsHeapLimitIncreasedForDebugging()
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    HeapImpl* heap = static_cast<HeapImpl*>(impl->ii.heap());
    return heap->IsHeapLimitIncreasedForDebugging();
}

/**
 * Allows the host application to provide the address of a function that is
 * notified each time code is added, moved or removed.
 *
 * \param options options for the JIT code event handler.
 * \param event_handler the JIT code event handler, which will be invoked
 *     each time code is added, moved or removed.
 * \note \p event_handler won't get notified of existent code.
 * \note since code removal notifications are not currently issued, the
 *     \p event_handler may get notifications of code that overlaps earlier
 *     code notifications. This happens when code areas are reused, and the
 *     earlier overlapping code areas should therefore be discarded.
 * \note the events passed to \p event_handler and the strings they point to
 *     are not guaranteed to live past each call. The \p event_handler must
 *     copy strings and other parameters it needs to keep around.
 * \note the set of events declared in JitCodeEvent::EventType is expected to
 *     grow over time, and the JitCodeEvent structure is expected to accrue
 *     new members. The \p event_handler function must ignore event codes
 *     it does not recognize to maintain future compatibility.
 * \note Use Isolate::CreateParams to get events for code executed during
 *     Isolate setup.
 */
void Isolate::SetJitCodeEventHandler(JitCodeEventOptions options,
                            JitCodeEventHandler event_handler)
{
    assert(0);
}

/**
 * Modifies the stack limit for this Isolate.
 *
 * \param stack_limit An address beyond which the Vm's stack may not grow.
 *
 * \note  If you are using threads then you should hold the V8::Locker lock
 *     while setting the stack limit and you must set a non-default stack
 *     limit separately for each thread.
 */
void Isolate::SetStackLimit(uintptr_t stack_limit)
{
    printf ("FIXME: Isolate::SetStackLimit\n");
}

/**
 * Returns a memory range that can potentially contain jitted code.
 *
 * On Win64, embedders are advised to install function table callbacks for
 * these ranges, as default SEH won't be able to unwind through jitted code.
 *
 * The first page of the code range is reserved for the embedder and is
 * committed, writable, and executable.
 *
 * Might be empty on other platforms.
 *
 * https://code.google.com/p/v8/issues/detail?id=3598
 */
void Isolate::GetCodeRange(void** start, size_t* length_in_bytes)
{
    assert(0);
}

/** Set the callback to invoke in case of fatal errors. */
void Isolate::SetFatalErrorHandler(FatalErrorCallback that)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_fatal_error_callback = that;
}

/** Set the callback to invoke in case of OOM errors. */
void Isolate::SetOOMErrorHandler(OOMErrorCallback that)
{
    assert(0);
}

/**
 * Set the callback to invoke to check if code generation from
 * strings should be allowed.
 */
void Isolate::SetAllowCodeGenerationFromStringsCallback(
                                               FreshNewAllowCodeGenerationFromStringsCallback callback)
{
    assert(0);
}

/**
 * Embedder over{ride|load} injection points for wasm APIs. The expectation
 * is that the embedder sets them at most once.
 */
void Isolate::SetWasmModuleCallback(ExtensionCallback callback)
{
    assert(0);
}
void Isolate::SetWasmInstanceCallback(ExtensionCallback callback)
{
    assert(0);
}

void Isolate::SetWasmCompileStreamingCallback(ApiImplementationCallback callback)
{
    assert(0);
}

/**
 * Check if V8 is dead and therefore unusable.  This is the case after
 * fatal errors such as out-of-memory situations.
 */
bool Isolate::IsDead()
{
    assert(0);
    return false;
}

/**
 * Adds a message listener (errors only).
 *
 * The same message listener can be added more than once and in that
 * case it will be called more than once for each message.
 *
 * If data is specified, it will be passed to the callback when it is called.
 * Otherwise, the exception object will be passed to the callback instead.
 */
bool Isolate::AddMessageListener(MessageCallback that,
                        Local<Value> data)
{
    return AddMessageListenerWithErrorLevel(that, kMessageError, data);
}

/**
 * Adds a message listener.
 *
 * The same message listener can be added more than once and in that
 * case it will be called more than once for each message.
 *
 * If data is specified, it will be passed to the callback when it is called.
 * Otherwise, the exception object will be passed to the callback instead.
 *
 * A listener can listen for particular error levels by providing a mask.
 */
bool Isolate::AddMessageListenerWithErrorLevel(MessageCallback that,
                                      int message_levels,
                                      Local<Value> data)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    HandleScope scope(this);
    Local<Context> context = V82JSC::OperatingContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    internal::MessageListener listener;
    listener.callback_ = that;
    listener.message_levels_ = message_levels;
    if (data.IsEmpty()) {
        listener.data_ = 0;
    } else {
        listener.data_ = V82JSC::ToJSValueRef(data, context);
        JSValueProtect(ctx, listener.data_);
    }
    
    iso->m_message_listeners.push_back(listener);
    
    return true;
}

/**
 * Remove all message listeners from the specified callback function.
 */
void Isolate::RemoveMessageListeners(MessageCallback that)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    HandleScope scope(this);
    Local<Context> context = V82JSC::OperatingContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    for (auto i=iso->m_message_listeners.begin(); i!=iso->m_message_listeners.end(); ) {
        if (i->callback_ == that) {
            if (i->data_) JSValueUnprotect(ctx, i->data_);
            iso->m_message_listeners.erase(i);
        } else {
            ++i;
        }
    }
}

/** Callback function for reporting failed access checks.*/
void Isolate::SetFailedAccessCheckCallbackFunction(FailedAccessCheckCallback)
{
    
}

/**
 * Tells V8 to capture current stack trace when uncaught exception occurs
 * and report it to the message listeners. The option is off by default.
 */
void Isolate::SetCaptureStackTraceForUncaughtExceptions(
                                               bool capture, int frame_limit,
                                               StackTrace::StackTraceOptions options)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_capture_stack_trace_for_uncaught_exceptions = capture;
}

/**
 * Iterates through all external resources referenced from current isolate
 * heap.  GC is not invoked prior to iterating, therefore there is no
 * guarantee that visited objects are still alive.
 */
void Isolate::VisitExternalResources(ExternalResourceVisitor* visitor)
{
    assert(0);
}

/**
 * Iterates through all the persistent handles in the current isolate's heap
 * that have class_ids.
 */
void Isolate::VisitHandlesWithClassIds(PersistentHandleVisitor* visitor)
{
    assert(0);
}

/**
 * Iterates through all the persistent handles in the current isolate's heap
 * that have class_ids and are candidates to be marked as partially dependent
 * handles. This will visit handles to young objects created since the last
 * garbage collection but is free to visit an arbitrary superset of these
 * objects.
 */
void Isolate::VisitHandlesForPartialDependence(PersistentHandleVisitor* visitor)
{
    assert(0);
}

/**
 * Iterates through all the persistent handles in the current isolate's heap
 * that have class_ids and are weak to be marked as inactive if there is no
 * pending activity for the handle.
 */
void Isolate::VisitWeakHandles(PersistentHandleVisitor* visitor)
{
    assert(0);
}

/**
 * Check if this isolate is in use.
 * True if at least one thread Enter'ed this isolate.
 */
bool Isolate::IsInUse()
{
    assert(0);
    return false;
}

/**
 * Set whether calling Atomics.wait (a function that may block) is allowed in
 * this isolate. This can also be configured via
 * CreateParams::allow_atomics_wait.
 */
void Isolate::SetAllowAtomicsWait(bool allow)
{
    assert(0);
}

void Isolate::ReportExternalAllocationLimitReached()
{
    printf ("FIXME! Isolate::ReportExternalAllocationLimitReached()\n");
}

Isolate::DisallowJavascriptExecutionScope::DisallowJavascriptExecutionScope(Isolate* isolate, OnFailure on_failure)
{
    assert(0);
}
Isolate::DisallowJavascriptExecutionScope::~DisallowJavascriptExecutionScope()
{
    assert(0);
}

Isolate::AllowJavascriptExecutionScope::AllowJavascriptExecutionScope(Isolate* isolate)
{
    assert(0);
}
Isolate::AllowJavascriptExecutionScope::~AllowJavascriptExecutionScope()
{
    assert(0);
}

Isolate::SuppressMicrotaskExecutionScope::SuppressMicrotaskExecutionScope(Isolate* isolate) : isolate_(nullptr)
{
    assert(0);
}
Isolate::SuppressMicrotaskExecutionScope::~SuppressMicrotaskExecutionScope()
{
    assert(0);
}
