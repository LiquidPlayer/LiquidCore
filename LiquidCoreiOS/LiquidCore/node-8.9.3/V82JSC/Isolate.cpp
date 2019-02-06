/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "Isolate.h"
#include "V82JSC.h"
#include "JSHeapFinalizerPrivate.h"
#include "JSMarkingConstraintPrivate.h"
#include "JSWeakRefPrivate.h"

using namespace v8;
using V82JSC_HeapObject::HeapImpl;

#define H V82JSC_HeapObject

#define DEF(T,V,F) \
v8::internal::Object ** V;
struct Roots {
    STRONG_ROOT_LIST(DEF)
};
#ifdef DEBUG
#define DECLARE_FIELDS(type,name,v) \
const intptr_t v8::internal::Isolate::name##_debug_offset_ = (reinterpret_cast<intptr_t>(&(reinterpret_cast<v8::internal::Isolate*>(16)->name##_)) - 16);
ISOLATE_INIT_LIST(DECLARE_FIELDS);
#endif

std::mutex IsolateImpl::s_isolate_mutex;
std::map<JSGlobalContextRef, IsolateImpl*> IsolateImpl::s_context_to_isolate_map;

static void triggerGarbageCollection(IsolateImpl* iso)
{
    Isolate* isolate = V82JSC::ToIsolate(iso);
    if (!iso->m_pending_collection) {
        iso->m_pending_collection = true;
        isolate->EnqueueMicrotask([](void *data){
            reinterpret_cast<IsolateImpl*>(data)->CollectGarbage();
            reinterpret_cast<IsolateImpl*>(data)->m_pending_collection = false;
        }, iso);
    }
}

static void MarkingConstraintCallback(JSMarkerRef marker, void *userData)
{
    IsolateImpl *impl = (IsolateImpl*)userData;
    impl->performIncrementalMarking(marker, impl->m_near_death);
    impl->m_pending_prologue = true;
    triggerGarbageCollection(impl);
}

static void HeapFinalizerCallback(JSContextGroupRef grp, void *userData)
{
    IsolateImpl *impl = (IsolateImpl*)userData;
    impl->m_pending_epilogue = true;
    triggerGarbageCollection(impl);
}

std::atomic<bool> IsolateImpl::s_isLockerActive(false);

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
    memset((void*)impl, 0, sizeof(IsolateImpl));
    Isolate * isolate = V82JSC::ToIsolate(impl);

    reinterpret_cast<internal::Isolate*>(isolate)->Init((v8::internal::Deserializer *)&params);

    HeapImpl* heap = static_cast<HeapImpl*>(impl->ii.heap());
    heap->m_heap_top = nullptr;
    heap->m_allocated = 0;
    heap->SetUp();
    
    impl->m_global_symbols = std::map<std::string, JSValueRef>();
    impl->m_private_symbols = std::map<std::string, JSValueRef>();
    impl->m_global_contexts = std::map<JSGlobalContextRef, Copyable(Context)>();
    impl->m_exec_maps = std::map<JSGlobalContextRef, std::map<const char *, JSObjectRef>>();
    impl->m_message_listeners = std::vector<internal::MessageListener>();
    impl->m_gc_prologue_callbacks = std::vector<IsolateImpl::GCCallbackStruct>();
    impl->m_gc_epilogue_callbacks = std::vector<IsolateImpl::GCCallbackStruct>();
    impl->m_near_death = std::map<void*, JSObjectRef>();
    impl->m_second_pass_callbacks = std::vector<internal::SecondPassCallback>();
    impl->m_external_strings = std::map<JSValueRef, Copyable(v8::WeakExternalString)>();
    impl->m_internalized_strings = std::map<JSStringRef, Copyable(v8::String)*>();
    impl->m_microtask_queue = std::vector<IsolateImpl::EnqueuedMicrotask>();
    impl->m_microtasks_completed_callback = std::vector<v8::MicrotasksCompletedCallback>();
    impl->m_microtasks_policy = v8::MicrotasksPolicy::kAuto;
    impl->m_jsobjects = std::map<JSObjectRef, ValueImpl*>();
    impl->m_before_call_callbacks = std::vector<BeforeCallEnteredCallback>();
    impl->m_call_completed_callbacks = std::vector<CallCompletedCallback>();
    impl->m_eternal_handles = std::vector<Copyable(v8::Value) *>();

    impl->m_locker = nullptr;
    impl->m_entered_count = 0;
    
    new (&impl->m_isolate_lock) std::mutex();
    new (&impl->m_pending_interrupt_mutex) std::mutex();
    new (&impl->m_handlewalk_lock) std::mutex();
    impl->m_pending_interrupts = std::vector<IsolateImpl::PendingInterrupt>();

    HandleScope scope(isolate);
    
    impl->m_params = params;
    
    impl->m_group = JSContextGroupCreate();
    
    // Poll every second during script execution to see if there are any interrupts
    // pending
    JSContextGroupSetExecutionTimeLimit(impl->m_group, 1, IsolateImpl::PollForInterrupts, impl);
    
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
    impl->m_promise_resolver_map = H::Map<H::Value>::New(impl, internal::JS_PROMISE_TYPE);
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
    roots->external_one_byte_string_map = reinterpret_cast<internal::Object**> (H::ToV8Map(impl->m_external_one_byte_string_map));
    roots->external_string_map = reinterpret_cast<internal::Object**> (H::ToV8Map(impl->m_external_string_map));

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
    auto thread = IsolateImpl::PerThreadData::Get(impl);
    thread->m_scheduled_exception =
        impl->ii.heap()->root(v8::internal::Heap::RootListIndex::kTheHoleValueRootIndex);
    
    return reinterpret_cast<v8::Isolate*>(isolate);
}

#define EXTERNAL_MEMORY_SIZE (512 * 1024)

bool internal::Heap::SetUp()
{
    IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(isolate());
    incremental_marking_ = &iso->incremental_marking_;
    
    void *memptr;
    posix_memalign(&memptr, EXTERNAL_MEMORY_SIZE, EXTERNAL_MEMORY_SIZE);
    external_memory_ = reinterpret_cast<uint64_t>(memptr);
    external_memory_limit_ = external_memory_ + EXTERNAL_MEMORY_SIZE;
    
    return true;
}

bool IsolateImpl::PollForInterrupts(JSContextRef ctx, void* context)
{
    IsolateImpl* iso = (IsolateImpl*)context;
    // Reset poll to one-second
    JSContextGroupSetExecutionTimeLimit(iso->m_group, 1, IsolateImpl::PollForInterrupts, iso);
    bool empty = false;
    bool terminate = iso->m_terminate_execution;
    
    if (terminate) {
        return true;
    }
    
    iso->m_pending_interrupt_mutex.lock();
    empty = iso->m_pending_interrupts.empty();
    while (!empty) {
        IsolateImpl::PendingInterrupt interrupt = iso->m_pending_interrupts.front();
        iso->m_pending_interrupt_mutex.unlock();
        interrupt.m_callback(V82JSC::ToIsolate(iso), interrupt.m_data);
        iso->m_pending_interrupt_mutex.lock();
        iso->m_pending_interrupts.erase(iso->m_pending_interrupts.begin());
        empty = iso->m_pending_interrupts.empty();
    }
    iso->m_pending_interrupt_mutex.unlock();
    return false;
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
            m_external_strings.erase(i++);
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

std::mutex IsolateImpl::s_thread_data_mutex;
std::map<size_t, IsolateImpl::PerThreadData*> IsolateImpl::s_thread_data;
IsolateImpl::PerThreadData* IsolateImpl::PerThreadData::Get()
{
    std::unique_lock<std::mutex> lock(s_thread_data_mutex);
    size_t hash = std::hash<std::thread::id>()(std::this_thread::get_id());
    size_t has = s_thread_data.count(hash);
    if (has == 0) {
        PerThreadData *data = new PerThreadData();
        s_thread_data[hash] = data;
        return data;
    } else {
        return s_thread_data[hash];
    }
}
void IsolateImpl::PerThreadData::EnterThreadContext(v8::Isolate *isolate)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(isolate);
    size_t hash = std::hash<std::thread::id>()(std::this_thread::get_id());
    if (hash != iso->m_current_thread_hash) {
        auto thread = Get(iso);
        if (iso->m_current_thread_context != thread) {
            if (iso->m_current_thread_context) {
                memcpy(&iso->m_current_thread_context->m_handle_scope_data,
                       iso->ii.handle_scope_data(), sizeof(HandleScopeData));
            }
            iso->m_current_thread_context = thread;
            memcpy(iso->ii.handle_scope_data(), &iso->m_current_thread_context->m_handle_scope_data,
                   sizeof(HandleScopeData));
            iso->m_scope_stack = &thread->m_scope_stack;
        }
        iso->m_current_thread_hash = hash;
    }
}


/**
 * Returns the entered isolate for the current thread or NULL in
 * case there is no current isolate.
 *
 * This method must not be invoked before V8::Initialize() was invoked.
 */
Isolate* Isolate::GetCurrent()
{
    auto thread = IsolateImpl::PerThreadData::Get();
    if (thread->m_entered_isolates.empty()) return nullptr;
    return thread->m_entered_isolates.back();
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
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    iso->m_on_uncaught_exception_callback = callback;
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
    static auto collectGarbage = [](IsolateImpl* iso) {
        /*
        iso->CollectGarbage();
        for (auto i=iso->m_global_contexts.begin(); i!=iso->m_global_contexts.end(); ++i) {
            JSGarbageCollect(V82JSC::ToContextImpl(i->second.Get(V82JSC::ToIsolate(iso)))->m_ctxRef);
        }
        */
        V82JSC::ToIsolate(iso)->RequestGarbageCollectionForTesting(GarbageCollectionType::kFullGarbageCollection);
    };
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    iso->m_should_optimize_for_memory_usage = (level > MemoryPressureLevel::kNone);
    if (iso->m_should_optimize_for_memory_usage) {
        if (!v8::Locker::IsLocked(this)) {
            RequestInterrupt([](Isolate* isolate, void* data) {
                isolate->EnqueueMicrotask([](void *data){
                    collectGarbage(reinterpret_cast<IsolateImpl*>(data));
                }, isolate);
            }, nullptr);
        } else {
            printf ("Immediately collecting garbage due to memory pressure\n");
            collectGarbage(iso);
        }
    }
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
    auto thread = IsolateImpl::PerThreadData::Get();
    thread->m_entered_isolates.push_back(this);
    V82JSC::ToIsolateImpl(this)->m_entered_count ++;
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
    CHECK_EQ(this, Isolate::GetCurrent());
    auto thread = IsolateImpl::PerThreadData::Get();
    thread->m_entered_isolates.pop_back();
    V82JSC::ToIsolateImpl(this)->m_entered_count --;
}

void IsolateImpl::EnterContext(Local<v8::Context> ctx)
{
    auto thread = IsolateImpl::PerThreadData::Get(this);
    Copyable(v8::Context) persist(V82JSC::ToIsolate(this), ctx);
    thread->m_context_stack.push(persist);
    persist.Reset();
}

void IsolateImpl::ExitContext(Local<v8::Context> ctx)
{
    auto thread = IsolateImpl::PerThreadData::Get(this);
    assert(thread->m_context_stack.size());
    JSContextRef top = V82JSC::ToContextRef(thread->m_context_stack.top().Get(V82JSC::ToIsolate(this)));
    JSContextRef newctx = V82JSC::ToContextRef(ctx);
    assert(top == newctx);

    thread->m_context_stack.top().Reset();
    thread->m_context_stack.pop();
}

/**
 * Disposes the isolate.  The isolate must not be entered by any
 * thread to be disposable.
 */

JS_EXPORT void JSContextGroupRemoveMarkingConstraint(JSContextGroupRef, JSMarkingConstraint, void *userData);

IsolateImpl::PerIsolateThreadData::~PerIsolateThreadData() {
    while (m_handlers) {
        TryCatchCopy *tcc = reinterpret_cast<TryCatchCopy*>(m_handlers);
        m_handlers = tcc->next_;
        delete(tcc);
    }
}

void Isolate::Dispose()
{
    IsolateImpl *isolate = (IsolateImpl *)this;
    if (isolate->m_entered_count) {
        if (isolate->m_fatal_error_callback) {
            isolate->m_fatal_error_callback("Isolate::Dispose()", "Attempting to dispose an entered isolate.");
        } else {
            FATAL("Attempting to dispose an entered isolate.");
        }
        return;
    }

    JSContextGroupClearExecutionTimeLimit(isolate->m_group);

    // Hmm.  There is no JSContextGroupRemoveMarkingConstraint() equivalent
    JSContextGroupRemoveHeapFinalizer(isolate->m_group, HeapFinalizerCallback, isolate);
    {
        HandleScope scope(V82JSC::ToIsolate(isolate));
        auto deleted = std::map<String::ExternalStringResourceBase*, bool>();
        while (!isolate->m_external_strings.empty()) {
            Local<WeakExternalString> wes = isolate->m_external_strings.begin()->second.Get(V82JSC::ToIsolate(isolate));
            WeakExternalStringImpl *impl = V82JSC::ToImpl<WeakExternalStringImpl>(wes);
            impl->FinalizeExternalString();
            isolate->m_external_strings.erase(isolate->m_external_strings.begin());
        }
    }
    for (auto i=isolate->m_internalized_strings.begin(); i!=isolate->m_internalized_strings.end(); ++i) {
        i->second->Reset();
        delete i->second;
    }
    isolate->m_internalized_strings.clear();

    JSContextGroupRelease(isolate->m_group);
    {
        std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
        for (auto i=isolate->m_global_contexts.begin(); i != isolate->m_global_contexts.end(); i++) {
            IsolateImpl::s_context_to_isolate_map.erase(i->first);
        }
    }
    isolate->m_global_contexts.clear();
    isolate->m_exec_maps.clear();
    isolate->m_nullContext.Reset();
    isolate->m_microtask_queue.clear();
    isolate->m_microtasks_completed_callback.clear();
    isolate->m_pending_interrupts.clear();
    isolate->m_global_symbols.clear();
    isolate->m_private_symbols.clear();
    isolate->m_message_listeners.clear();
    isolate->m_before_call_callbacks.clear();
    isolate->m_call_completed_callbacks.clear();
    isolate->m_gc_prologue_callbacks.clear();
    isolate->m_gc_epilogue_callbacks.clear();
    isolate->m_second_pass_callbacks.clear();
    isolate->m_jsobjects.clear();
    
    for (auto i=isolate->m_eternal_handles.begin(); i!=isolate->m_eternal_handles.end(); ++i) {
        (*i)->Reset();
        delete (*i);
    }
    isolate->m_eternal_handles.clear();

    auto thread = IsolateImpl::PerThreadData::Get(isolate);
    for (auto i=IsolateImpl::s_thread_data.begin(); i!=IsolateImpl::s_thread_data.end(); i++) {
        if (i->second->m_isolate_data.count(isolate) != 0) {
            i->second->m_isolate_data.erase(isolate);
        }
    }
    delete thread;

    {
        std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
        for (auto i=IsolateImpl::s_context_to_isolate_map.begin(); i!=IsolateImpl::s_context_to_isolate_map.end(); ) {
            if (i->second == isolate) {
                IsolateImpl::s_context_to_isolate_map.erase(i++);
            } else {
                i++;
            }
        }
    }
    
    // Finally, blitz the global handles and the heap
    void *memptr = reinterpret_cast<void*>(isolate->ii.heap()->external_memory());
    free(memptr);

    isolate->ii.global_handles()->TearDown();
    V82JSC_HeapObject::HeapAllocator::TearDown(isolate);
    
    if (isolate->m_locker) delete isolate->m_locker;
    
    // And now the isolate is done
    free(isolate);
}

// This method is called once JSC's garbage collector has run
void IsolateImpl::CollectGarbage()
{
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(this);
    if (iso->m_in_gc++) return;
    
    H::HeapAllocator::CollectGarbage(this);

    iso->TriggerGCPrologue();
    iso->CollectExternalStrings();
    
    iso->TriggerGCEpilogue();
    
    iso->m_in_gc = 0;
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
    // FIXME: assert(0);
    return nullptr;
}

/** Returns true if this isolate has a current context. */
bool Isolate::InContext()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(this);
    auto thread = IsolateImpl::PerThreadData::Get(impl);
    return thread->m_context_stack.size() != 0;
}

/**
 * Returns the context of the currently running JavaScript, or the context
 * on the top of the stack if no JavaScript is running.
 */
Local<Context> Isolate::GetCurrentContext()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(this);
    auto thread = IsolateImpl::PerThreadData::Get(impl);
    if (!thread->m_context_stack.size()) {
        return Local<Context>();
    }
    
    return Local<Context>::New(this, thread->m_context_stack.top());
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
    auto thread = IsolateImpl::PerThreadData::Get(impl);
    
    if (exception.IsEmpty()) {
        thread->m_scheduled_exception = impl->ii.heap()->root(v8::internal::Heap::RootListIndex::kUndefinedValueRootIndex);
    } else {
        thread->m_scheduled_exception = * reinterpret_cast<internal::Object**>(*exception);
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
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(this);
    iso->m_terminate_execution = true;
    JSContextGroupSetExecutionTimeLimit(iso->m_group, 0, IsolateImpl::PollForInterrupts, iso);
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
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(this);
    return iso->m_terminate_execution;
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
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(this);
    iso->m_pending_interrupt_mutex.lock();
    iso->m_pending_interrupts.push_back(IsolateImpl::PendingInterrupt(callback,data));
    iso->m_pending_interrupt_mutex.unlock();
    // If we haven't started the script yet, this will force an interrupt immediately
    // The interval will get set back to 1 second after the first interrupt
    /*
    JSContextGroupSetExecutionTimeLimit(iso->m_group, 0, IsolateImpl::PollForInterrupts, iso);
    */
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
    H::HeapAllocator::CollectGarbage(iso);
    
    // Next, trigger garbage collection in JSC
    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
    }
    
    iso->TriggerGCPrologue();
    iso->TriggerGCFirstPassPhantomCallbacks();
    
    // Next, trigger garbage collection in JSC (do it twice -- sometimes the first doesn't finish the job)
    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
        JSSynchronousGarbageCollectForDebugging(i->first);
    }
    
    iso->TriggerGCFirstPassPhantomCallbacks();
    iso->CollectExternalStrings();
    H::HeapAllocator::CollectGarbage(iso);

    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
    }

    // Second pass, clear V82JSC garbage again in case any weak references were cleared
    H::HeapAllocator::CollectGarbage(iso);
    iso->TriggerGCEpilogue();
    
    iso->m_in_gc = 0;
}

/**
 * Set the callback to invoke for logging event.
 */
void Isolate::SetEventLogger(LogEventCallback that)
{
    reinterpret_cast<internal::Isolate*>(this)->event_logger_ = that;
}

/**
 * Adds a callback to notify the host application right before a script
 * is about to run. If a script re-enters the runtime during executing, the
 * BeforeCallEnteredCallback is invoked for each re-entrance.
 * Executing scripts inside the callback will re-trigger the callback.
 */
void Isolate::AddBeforeCallEnteredCallback(BeforeCallEnteredCallback callback)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    RemoveBeforeCallEnteredCallback(callback);
    iso->m_before_call_callbacks.push_back(callback);
}

/**
 * Removes callback that was installed by AddBeforeCallEnteredCallback.
 */
void Isolate::RemoveBeforeCallEnteredCallback(BeforeCallEnteredCallback callback)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    for (auto i=iso->m_before_call_callbacks.begin(); i!=iso->m_before_call_callbacks.end(); ) {
        if ( *i == callback ) iso->m_before_call_callbacks.erase(i);
        else ++i;
    }
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
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    RemoveCallCompletedCallback(callback);
    iso->m_call_completed_callbacks.push_back(callback);
}

/**
 * Removes callback that was installed by AddCallCompletedCallback.
 */
void Isolate::RemoveCallCompletedCallback(CallCompletedCallback callback)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    for (auto i=iso->m_call_completed_callbacks.begin(); i!=iso->m_call_completed_callbacks.end(); ) {
        if ( *i == callback ) iso->m_call_completed_callbacks.erase(i);
        else ++i;
    }
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
    //FIXME! assert(0);
}

/**
 * Experimental: Runs the Microtask Work Queue until empty
 * Any exceptions thrown by microtask callbacks are swallowed.
 */
void Isolate::RunMicrotasks()
{
    if (v8::MicrotasksScope::IsRunningMicrotasks(this)) return;
    
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    if (iso->m_suppress_microtasks) return;
    
    if (!iso->m_microtask_queue.empty()) {
        HandleScope scope(this);
        TryCatch try_catch(this);
        bool running = iso->m_running_microtasks;
        iso->m_running_microtasks = true;
        while (!iso->m_microtask_queue.empty()) {
            auto i = iso->m_microtask_queue.begin();
            IsolateImpl::EnqueuedMicrotask& microtask = *i;
            if (!microtask.m_callback.IsEmpty()) {
                Local<Function> task = microtask.m_callback.Get(this);
                task->Call(V82JSC::OperatingContext(this), Local<Value>(), 0, nullptr).FromMaybe(Local<Value>());
            } else {
                microtask.m_native_callback(microtask.m_data);
            }
            iso->m_microtask_queue.erase(iso->m_microtask_queue.begin());
        }
        iso->m_running_microtasks = running;
        if (!running) {
            for (auto i=iso->m_microtasks_completed_callback.begin();
                 i!=iso->m_microtasks_completed_callback.end(); ++i) {
                (*i)(this);
            }
        }
    }
}

/**
 * Experimental: Enqueues the callback to the Microtask Work Queue
 */
void Isolate::EnqueueMicrotask(Local<Function> microtask)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    iso->m_microtask_queue.push_back(IsolateImpl::EnqueuedMicrotask(this, microtask));
}

/**
 * Experimental: Enqueues the callback to the Microtask Work Queue
 */
void Isolate::EnqueueMicrotask(MicrotaskCallback microtask, void* data)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    iso->m_microtask_queue.push_back(IsolateImpl::EnqueuedMicrotask(microtask, data));
}

/**
 * Experimental: Controls how Microtasks are invoked. See MicrotasksPolicy
 * for details.
 */
void Isolate::SetMicrotasksPolicy(MicrotasksPolicy policy)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    iso->m_microtasks_policy = policy;
}

/**
 * Experimental: Returns the policy controlling how Microtasks are invoked.
 */
MicrotasksPolicy Isolate::GetMicrotasksPolicy() const
{
    const IsolateImpl* iso = reinterpret_cast<const IsolateImpl*>(this);
    return iso->m_microtasks_policy;
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
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    iso->m_microtasks_completed_callback.push_back(callback);
}

/**
 * Removes callback that was installed by AddMicrotasksCompletedCallback.
 */
void Isolate::RemoveMicrotasksCompletedCallback(MicrotasksCompletedCallback callback)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(this);
    for (auto i=iso->m_microtasks_completed_callback.begin(); i!=iso->m_microtasks_completed_callback.end(); ) {
        if (*i == callback) iso->m_microtasks_completed_callback.erase(i);
        else ++i;
    }
}

MicrotasksScope::MicrotasksScope(Isolate* isolate, Type type) :
    isolate_(reinterpret_cast<internal::Isolate*>(isolate)),
    run_(type==MicrotasksScope::Type::kRunMicrotasks)
{
    if (run_) V82JSC::ToIsolateImpl(isolate)->m_run_microtasks_depth ++;
}
MicrotasksScope::~MicrotasksScope()
{
    if (run_ && --reinterpret_cast<IsolateImpl*>(isolate_)->m_run_microtasks_depth == 0) {
        reinterpret_cast<Isolate*>(isolate_)->RunMicrotasks();
    }
}

/**
 * Runs microtasks if no kRunMicrotasks scope is currently active.
 */
void MicrotasksScope::PerformCheckpoint(Isolate* isolate)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    if (iso->m_run_microtasks_depth == 0) {
        isolate->RunMicrotasks();
    }
}

/**
 * Returns current depth of nested kRunMicrotasks scopes.
 */
int MicrotasksScope::GetCurrentDepth(Isolate* isolate)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    return iso->m_run_microtasks_depth;
}

/**
 * Returns true while microtasks are being executed.
 */
bool MicrotasksScope::IsRunningMicrotasks(Isolate* isolate)
{
    return V82JSC::ToIsolateImpl(isolate)->m_running_microtasks;
}

/**
 * Sets a callback for counting the number of times a feature of V8 is used.
 */
void Isolate::SetUseCounterCallback(UseCounterCallback that)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_use_counter_callback = that;
}

/**
 * Enables the host application to provide a mechanism for recording
 * statistics counters.
 */
void Isolate::SetCounterFunction(CounterLookupCallback that)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_counter_lookup_callback = that;
}

/**
 * Enables the host application to provide a mechanism for recording
 * histograms. The CreateHistogram function returns a
 * histogram which will later be passed to the AddHistogramSample
 * function.
 */
void Isolate::SetCreateHistogramFunction(CreateHistogramCallback that)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_create_histogram_callback = that;
}
void Isolate::SetAddHistogramSampleFunction(AddHistogramSampleCallback that)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_add_histogram_sample_callback = that;
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
    RequestGarbageCollectionForTesting(GarbageCollectionType::kFullGarbageCollection);

    return true;
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
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    impl->m_allow_code_gen_callback = callback;
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
void Isolate::SetFailedAccessCheckCallbackFunction(FailedAccessCheckCallback callback)
{
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    iso->m_failed_access_check_callback = callback;
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
    HandleScope scope(this);
    
    IsolateImpl *iso = V82JSC::ToIsolateImpl(this);
    for(auto i=iso->m_external_strings.begin(); i!=iso->m_external_strings.end(); ++i) {
        auto s = i->second.Get(this);
        H::WeakExternalString* wes = static_cast<H::WeakExternalString*>(H::FromHeapPointer(*(internal::Object**)*s));
        visitor->VisitExternalString(ValueImpl::New(V82JSC::ToContextImpl(iso->m_nullContext.Get(this)),
                                                    wes->m_value).As<String>());
    }
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
    reinterpret_cast<internal::Isolate*>(this)->allow_atomics_wait_ = allow;
}

void Isolate::ReportExternalAllocationLimitReached()
{
    printf ("FIXME! Isolate::ReportExternalAllocationLimitReached()\n");
}

struct DisallowScopeImpl {
    DisallowScopeImpl(Isolate* isolate)
    {
        m_isolate = V82JSC::ToIsolateImpl(isolate);
        m_prev_failure = m_isolate->m_on_failure;
        m_prev_disallow = m_isolate->m_disallow_js;
    }
    IsolateImpl* m_isolate;
    Isolate::DisallowJavascriptExecutionScope::OnFailure m_prev_failure;
    bool m_prev_disallow;
};

Isolate::DisallowJavascriptExecutionScope::DisallowJavascriptExecutionScope(Isolate* isolate, OnFailure on_failure)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    internal_ = (void*)new DisallowScopeImpl(isolate);
    iso->m_disallow_js = true;
    iso->m_on_failure = on_failure;
}
Isolate::DisallowJavascriptExecutionScope::~DisallowJavascriptExecutionScope()
{
    DisallowScopeImpl* impl = reinterpret_cast<DisallowScopeImpl*>(internal_);
    impl->m_isolate->m_disallow_js = impl->m_prev_disallow;
    impl->m_isolate->m_on_failure = impl->m_prev_failure;
    delete impl;
}

// Not sure what the intent of internal_assert_ and internal_throws_ are, but we will
// repurpose them
Isolate::AllowJavascriptExecutionScope::AllowJavascriptExecutionScope(Isolate* isolate)
{
    IsolateImpl* iso = V82JSC::ToIsolateImpl(isolate);
    this->internal_assert_ = (void*) iso->m_disallow_js;
    this->internal_throws_ = (void*) iso;
    iso->m_disallow_js = false;
}
Isolate::AllowJavascriptExecutionScope::~AllowJavascriptExecutionScope()
{
    IsolateImpl* iso = (IsolateImpl*) internal_throws_;
    iso->m_disallow_js = (bool)internal_assert_;
}

Isolate::SuppressMicrotaskExecutionScope::SuppressMicrotaskExecutionScope(Isolate* isolate) :
isolate_(reinterpret_cast<IsolateImpl*>(isolate)->m_suppress_microtasks ? nullptr : reinterpret_cast<internal::Isolate*>(isolate))
{
    V82JSC::ToIsolateImpl(isolate)->m_suppress_microtasks = true;
}
Isolate::SuppressMicrotaskExecutionScope::~SuppressMicrotaskExecutionScope()
{
    if (isolate_) {
        reinterpret_cast<IsolateImpl*>(isolate_)->m_suppress_microtasks = false;
    }
}
