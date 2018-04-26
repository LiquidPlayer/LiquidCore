//
//  Isolate.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

v8::Isolate *current = nullptr;

#define DECLARE_FIELDS(type,name,v) \
const intptr_t v8::internal::Isolate::name##_debug_offset_ = (reinterpret_cast<intptr_t>(&(reinterpret_cast<v8::internal::Isolate*>(16)->name##_)) - 16);
ISOLATE_INIT_LIST(DECLARE_FIELDS);

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
    IsolateImpl * isolate = (IsolateImpl *) malloc(sizeof (IsolateImpl));
    memset(isolate, 0, sizeof(IsolateImpl));
    
    isolate->m_params = params;
    
    isolate->m_group = JSContextGroupCreate();
    isolate->m_defaultContext = reinterpret_cast<ContextImpl*>(*Context::New(V82JSC::ToIsolate(isolate)));

    Primitive *undefined = ValueImpl::NewUndefined(reinterpret_cast<v8::Isolate*>(isolate));
    isolate->i.roots.undefined_value = reinterpret_cast<internal::Object **>((reinterpret_cast<intptr_t>(undefined) & ~3) +1);
    Primitive *the_hole = ValueImpl::NewUndefined(reinterpret_cast<v8::Isolate*>(isolate));
    isolate->i.roots.the_hole_value = reinterpret_cast<internal::Object **>((reinterpret_cast<intptr_t>(the_hole) & ~3) +1);
    Primitive *null = ValueImpl::NewNull(reinterpret_cast<v8::Isolate*>(isolate));
    isolate->i.roots.null_value = reinterpret_cast<internal::Object **>((reinterpret_cast<intptr_t>(null) & ~3) +1);
    Primitive *yup = ValueImpl::NewBoolean(reinterpret_cast<v8::Isolate*>(isolate), true);
    isolate->i.roots.true_value = reinterpret_cast<internal::Object **>((reinterpret_cast<intptr_t>(yup) & ~3) +1);
    Primitive *nope = ValueImpl::NewBoolean(reinterpret_cast<v8::Isolate*>(isolate), false);
    isolate->i.roots.false_value = reinterpret_cast<internal::Object **>((reinterpret_cast<intptr_t>(nope) & ~3) +1);
    
    JSStringRef empty_string = JSStringCreateWithUTF8CString("");
    Local<String> esv = ValueImpl::New(V82JSC::ToIsolate(isolate), empty_string);
    ValueImpl *es = V82JSC::ToImpl<ValueImpl>(esv);
    isolate->i.roots.empty_string = reinterpret_cast<internal::Object **>((reinterpret_cast<intptr_t>(es) & ~3) +1);;
    JSStringRelease(empty_string);
    
    isolate->m_global_symbols = std::map<std::string, JSValueRef>();
    isolate->m_private_symbols = std::map<std::string, JSValueRef>();

    return reinterpret_cast<v8::Isolate*>(isolate);
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

void IsolateImpl::EnterContext(v8::Context *ctx)
{
    current_context = ctx;
}

void IsolateImpl::ExitContext(v8::Context *ctx)
{
    // FIXME: Need a stack implementation
    current_context = nullptr;
}

/**
 * Disposes the isolate.  The isolate must not be entered by any
 * thread to be disposable.
 */
void Isolate::Dispose()
{
    IsolateImpl *isolate = (IsolateImpl *)this;
    JSContextGroupRelease(isolate->m_group);
    
    free(isolate);
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
    assert(0);
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
    return impl->current_context != nullptr;
}

/**
 * Returns the context of the currently running JavaScript, or the context
 * on the top of the stack if no JavaScript is running.
 */
Local<Context> Isolate::GetCurrentContext()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(this);
    return _local<Context>(impl->current_context).toLocal();
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
    JSValueRef excp = V82JSC::ToJSValueRef(exception, this);
    IsolateImpl* impl = V82JSC::ToIsolateImpl(this);
    impl->m_pending_exception = excp;

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
    assert(0);
}
void Isolate::AddGCPrologueCallback(GCCallback callback,
                           GCType gc_type_filter)
{
    assert(0);
}

/**
 * This function removes callback which was installed by
 * AddGCPrologueCallback function.
 */
void Isolate::RemoveGCPrologueCallback(GCCallbackWithData, void* data)
{
    assert(0);
}
void Isolate::RemoveGCPrologueCallback(GCCallback callback)
{
    assert(0);
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
void Isolate::AddGCEpilogueCallback(GCCallbackWithData callback, void* data,
                           GCType gc_type_filter)
{
    assert(0);
}
void Isolate::AddGCEpilogueCallback(GCCallback callback,
                           GCType gc_type_filter)
{
    assert(0);
}

/**
 * This function removes callback which was installed by
 * AddGCEpilogueCallback function.
 */
void Isolate::RemoveGCEpilogueCallback(GCCallbackWithData callback,
                              void* data)
{
    assert(0);
}
void Isolate::RemoveGCEpilogueCallback(GCCallback callback)
{
    assert(0);
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
    assert(0);
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
    assert(0);
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
    assert(0);
}

/**
 * Restores the original heap limit after IncreaseHeapLimitForDebugging().
 */
void Isolate::RestoreOriginalHeapLimit()
{
    assert(0);
}

/**
 * Returns true if the heap limit was increased for debugging and the
 * original heap limit was not restored yet.
 */
bool Isolate::IsHeapLimitIncreasedForDebugging()
{
    assert(0);
    return false;
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
    assert(0);
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
    assert(0);
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
    assert(0);
    return false;
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
    assert(0);
    return false;
}

/**
 * Remove all message listeners from the specified callback function.
 */
void Isolate::RemoveMessageListeners(MessageCallback that)
{
    assert(0);
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
    assert(0);
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
    assert(0);
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

/*
internal::Isolate::Isolate(bool enable_serializer)
    : embedder_data_(),
    entry_stack_(NULL),
    stack_trace_nesting_level_(0),
    incomplete_message_(NULL),
    bootstrapper_(NULL),
    runtime_profiler_(NULL),
    compilation_cache_(NULL),
    logger_(NULL),
    load_stub_cache_(NULL),
    store_stub_cache_(NULL),
    code_aging_helper_(NULL),
    deoptimizer_data_(NULL),
    deoptimizer_lazy_throw_(false),
    materialized_object_store_(NULL),
    capture_stack_trace_for_uncaught_exceptions_(false),
    stack_trace_for_uncaught_exceptions_frame_limit_(0),
    stack_trace_for_uncaught_exceptions_options_(StackTrace::kOverview),
    context_slot_cache_(NULL),
    descriptor_lookup_cache_(NULL),
    handle_scope_implementer_(NULL),
    unicode_cache_(NULL),
    allocator_(new AccountingAllocator()),
    inner_pointer_to_code_cache_(NULL),
    global_handles_(NULL),
    eternal_handles_(NULL),
    thread_manager_(NULL),
    setup_delegate_(NULL),
    regexp_stack_(NULL),
    date_cache_(NULL),
    call_descriptor_data_(NULL),
    // TODO(bmeurer) Initialized lazily because it depends on flags; can
    // be fixed once the default isolate cleanup is done.
    random_number_generator_(NULL),
    rail_mode_(PERFORMANCE_ANIMATION),
    promise_hook_or_debug_is_active_(false),
    promise_hook_(NULL),
    load_start_time_ms_(0),
    serializer_enabled_(enable_serializer),
    has_fatal_error_(false),
    initialized_from_snapshot_(false),
    is_tail_call_elimination_enabled_(true),
    is_isolate_in_background_(false),
    cpu_profiler_(NULL),
    heap_profiler_(NULL),
    code_event_dispatcher_(new CodeEventDispatcher()),
    function_entry_hook_(NULL),
    deferred_handles_head_(NULL),
    optimizing_compile_dispatcher_(NULL),
    stress_deopt_count_(0),
    next_optimization_id_(0),
    #if V8_SFI_HAS_UNIQUE_ID
    next_unique_sfi_id_(0),
    #endif
    is_running_microtasks_(false),
    use_counter_callback_(NULL),
    basic_block_profiler_(NULL),
    cancelable_task_manager_(new CancelableTaskManager()),
    wasm_compilation_manager_(new wasm::CompilationManager()),
    abort_on_uncaught_exception_callback_(NULL),
    total_regexp_code_generated_(0)
{
    
}

internal::Isolate::~Isolate()
{
    
}
*/

