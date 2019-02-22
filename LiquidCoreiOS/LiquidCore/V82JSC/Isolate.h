/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef Isolate_h
#define Isolate_h

#include "HeapObjects.h"
#include "JSMarkingConstraintPrivate.h"
#include <thread>

namespace V82JSC {
    struct Context;
    struct GlobalContext;
    struct UnboundScript;
    struct Script;
    struct Value;
    struct WeakValue;
    struct String;
    struct WeakExternalString;
    struct Message;
    struct FixedArray;
    struct Template;
    struct ObjectTemplate;
    struct FunctionTemplate;
    struct Prop;
    struct PropAccessor;
    struct ObjAccessor;
    struct Signature;
    struct IntrinsicProp;
    struct StackFrame;
    struct StackTrace;
    struct TrackedObject;
    struct Accessor;
}

// These aren't really V8 values, but we want to use V8 handles to manage their
// lifecycle, so we pretend.
namespace v8 {
    struct WeakExternalString : v8::Value {};
};

namespace v8 {
namespace internal {
    
using v8::InterruptCallback;
using v8::ExtensionCallback;
using v8::LogEventCallback;
using v8::AllowCodeGenerationFromStringsCallback;
using v8::ApiImplementationCallback;
using v8::PromiseRejectCallback;
using v8::FatalErrorCallback;
using v8::OOMErrorCallback;
using v8::BeforeCallEnteredCallback;
using v8::CallCompletedCallback;
using v8::MicrotasksCompletedCallback;
using v8::internal::Isolate;
    
#define kJSRegexpStaticOffsetsVectorSize v8::internal::Isolate::kJSRegexpStaticOffsetsVectorSize
#define kBMMaxShift v8::internal::Isolate::kBMMaxShift
#define kUC16AlphabetSize v8::internal::Isolate::kUC16AlphabetSize

#define H V82JSC

struct SecondPassCallback {
    JSObjectRef object_;
    v8::WeakCallbackInfo<void>::Callback callback_;
    void *param_;
    void *embedder_fields_[2];
    bool ready_to_call;
};
    
struct MessageListener {
    v8::MessageCallback callback_;
    JSValueRef data_;
    int message_levels_;
};
    
// This is a hack to avoid having to edit the V8 header files.  We do garbage collection
// differently and GlobalHandle struct is locked down as private.  This allows us to call
// back into GlobalHandle with a custom function.
typedef std::function<void(H::HeapContext&)> GetGlobalHandles;
typedef std::function<void(v8::internal::Object**, std::vector<v8::internal::SecondPassCallback>&,
                           JSObjectRef)> WeakObjectNearDeath;
typedef std::function<void(v8::Isolate*, v8::internal::SecondPassCallback&)> WeakHeapObjectFinalized;
typedef std::function<void(JSGlobalContextRef, JSObjectRef)> WeakJSObjectFinalized;
typedef std::function<void(JSMarkerRef, std::map<void*, JSObjectRef>&)> PerformIncrementalMarking;

struct IsolateImpl {
    v8::internal::Isolate ii;
    
    JSContextGroupRef m_group;
    v8::Persistent<v8::Context> m_nullContext;
    JSValueRef m_negative_zero;
    JSValueRef m_empty_string;
    JSValueRef m_private_symbol;
    JSObjectRef m_proxy_revocables;
    
    // Maps
    H::Map<H::TrackedObject> *m_tracked_object_map;
    H::Map<H::Value> *m_array_buffer_map;
    H::Map<H::Context> *m_context_map;
    H::Map<H::GlobalContext> *m_global_context_map;
    H::Map<H::FixedArray> *m_fixed_array_map;
    H::Map<H::String> *m_one_byte_string_map;
    H::Map<H::String> *m_string_map;
    H::Map<H::String> *m_external_string_map;
    H::Map<H::String> *m_external_one_byte_string_map;
    H::Map<H::String> *m_internalized_string_map;
    H::Map<H::String> *m_external_internalized_string_map;
    H::Map<H::String> *m_external_internalized_one_bye_string_map;
    H::Map<H::WeakExternalString> *m_weak_external_string_map;
    H::Map<H::WeakExternalString> *m_weak_external_one_byte_string_map;
    H::Map<H::Value> *m_value_map;
    H::Map<H::Value> *m_number_map;
    H::Map<H::Value> *m_symbol_map;
    H::Map<H::Value> *m_promise_resolver_map;
    H::Map<H::Signature> *m_signature_map;
    H::Map<H::FunctionTemplate> *m_function_template_map;
    H::Map<H::ObjectTemplate> *m_object_template_map;
    H::Map<H::Prop> *m_property_map;
    H::Map<H::PropAccessor> *m_property_accessor_map;
    H::Map<H::IntrinsicProp> *m_intrinsic_property_map;
    H::Map<H::Accessor> *m_accessor_map;
    H::Map<H::ObjAccessor> *m_object_accessor_map;
    H::Map<H::UnboundScript> *m_unbound_script_map;
    H::Map<H::Script> *m_script_map;
    H::Map<H::WeakValue> *m_weak_value_map;
    H::Map<H::StackFrame> *m_stack_frame_map;
    H::Map<H::StackTrace> *m_stack_trace_map;
    H::Map<H::Message> *m_message_map;

    std::map<JSObjectRef, V82JSC::Value*> m_jsobjects;
    
    std::map<std::string, JSValueRef> m_global_symbols;
    std::map<std::string, JSValueRef> m_private_symbols;
    
    v8::Isolate::CreateParams m_params;
    
    std::map<JSGlobalContextRef, v8::Persistent<v8::Context>> m_global_contexts;
    static std::mutex s_isolate_mutex;
    static std::map<JSGlobalContextRef, IsolateImpl*> s_context_to_isolate_map;
    
    std::map<JSGlobalContextRef, std::map<const char *, JSObjectRef>> m_exec_maps;

    bool m_pending_garbage_collection;

    v8::FatalErrorCallback m_fatal_error_callback;
    v8::CounterLookupCallback m_counter_lookup_callback;
    v8::CreateHistogramCallback m_create_histogram_callback;
    v8::AddHistogramSampleCallback m_add_histogram_sample_callback;
    v8::Isolate::UseCounterCallback m_use_counter_callback;
    v8::FailedAccessCheckCallback m_failed_access_check_callback;
    
    std::vector<MessageListener> m_message_listeners;
    bool m_capture_stack_trace_for_uncaught_exceptions;
    
    std::atomic<int> m_entered_count;
    struct GCCallbackStruct
    {
        v8::Isolate::GCCallback m_callback;
        v8::Isolate::GCCallbackWithData m_callback_with_data;
        v8::GCType m_gc_type_filter;
        void *m_data;
    };
    
    std::vector<GCCallbackStruct> m_gc_prologue_callbacks;
    std::vector<GCCallbackStruct> m_gc_epilogue_callbacks;
    std::map<void*, JSObjectRef> m_near_death;
    std::atomic<bool> m_pending_collection;
    bool m_pending_prologue;
    bool m_pending_epilogue;
    int m_in_gc;
    std::vector<SecondPassCallback> m_second_pass_callbacks;
    std::map<JSValueRef, v8::Persistent<v8::WeakExternalString>> m_external_strings;
    std::map<JSStringRef, v8::Persistent<v8::String>*> m_internalized_strings;
    
    std::recursive_mutex *m_locker;
    static std::atomic<bool> s_isLockerActive;
    std::atomic<int> m_locks;
    std::thread::id m_owner;
    
    std::mutex m_isolate_lock;
    std::mutex m_handlewalk_lock;

    struct PerIsolateThreadData
    {
        std::vector<int> m_unlock_counts;
        std::atomic<int> m_locks;
        std::vector<void*> m_locker_tokens;
        std::vector<void*> m_unlocker_tokens;
        
        v8::TryCatch *m_handlers;
        std::stack<v8::Local<v8::Script>> m_running_scripts;
        std::stack<v8::Persistent<v8::Context, v8::CopyablePersistentTraits<v8::Context>>> m_context_stack;
        int m_callback_depth;
        v8::internal::Object *m_scheduled_exception;
        JSValueRef m_verbose_exception;
        HandleScopeData m_handle_scope_data;
        std::vector<v8::HandleScope*> m_scope_stack;
        
        ~PerIsolateThreadData();
    };
    
    struct PerThreadData
    {
        PerThreadData() {}
        std::vector<v8::Isolate*> m_entered_isolates;
        std::map<IsolateImpl*, PerIsolateThreadData*> m_isolate_data;
        std::mutex m_mutex;

        static PerThreadData* Get();
        static PerIsolateThreadData* Get(IsolateImpl* iso)
        {
            PerThreadData* data = Get();
            std::unique_lock<std::mutex> lock(data->m_mutex);
            if (data->m_isolate_data.count(iso) == 0) {
                auto periso = new PerIsolateThreadData();
                periso->m_scheduled_exception =
                iso->ii.heap()->root(v8::internal::Heap::RootListIndex::kTheHoleValueRootIndex);
                data->m_isolate_data[iso] = periso;
            }
            auto periso = data->m_isolate_data[iso];
            return periso;
        }
        static void EnterThreadContext(v8::Isolate *isolate);
    };
    std::atomic<size_t> m_current_thread_hash;
    PerIsolateThreadData *m_current_thread_context;
    std::vector<v8::HandleScope*> *m_scope_stack;
    static std::mutex s_thread_data_mutex;
    static std::map<size_t, PerThreadData*> s_thread_data;
    
    struct PendingInterrupt {
        PendingInterrupt(InterruptCallback callback, void* data) : m_callback(callback), m_data(data) {}
        InterruptCallback m_callback;
        void *m_data;
    };
    std::mutex m_pending_interrupt_mutex;
    std::vector<PendingInterrupt> m_pending_interrupts;
    std::atomic<bool> m_terminate_execution;
    
    bool m_should_optimize_for_memory_usage;
    
    struct EnqueuedMicrotask {
        EnqueuedMicrotask(v8::Isolate* isolate, Local<Function> callback) {
            m_callback.Reset(isolate, callback);
            m_native_callback = nullptr;
            m_data = nullptr;
        }
        EnqueuedMicrotask(MicrotaskCallback callback, void* data) {
            m_callback.Reset();
            m_native_callback = callback;
            m_data = data;
        }
        v8::MicrotaskCallback m_native_callback;
        void *m_data;
        v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> m_callback;
    };
    std::vector<EnqueuedMicrotask> m_microtask_queue;
    bool m_running_microtasks;
    bool m_suppress_microtasks;
    v8::MicrotasksPolicy m_microtasks_policy;
    std::vector<v8::MicrotasksCompletedCallback> m_microtasks_completed_callback;
    int m_run_microtasks_depth;
    
    v8::FreshNewAllowCodeGenerationFromStringsCallback m_allow_code_gen_callback;
    bool m_disallow_js;
    v8::Isolate::DisallowJavascriptExecutionScope::OnFailure m_on_failure;
    
    std::vector<BeforeCallEnteredCallback> m_before_call_callbacks;
    std::vector<CallCompletedCallback> m_call_completed_callbacks;
    v8::Isolate::AbortOnUncaughtExceptionCallback m_on_uncaught_exception_callback;
    
    std::vector<v8::Persistent<v8::Value> *> m_eternal_handles;

    void EnterContext(v8::Local<v8::Context> ctx);
    void ExitContext(v8::Local<v8::Context> ctx);
    void GetActiveLocalHandles(H::HeapContext&);
    void CollectGarbage();
    void TriggerGCPrologue();
    void TriggerGCFirstPassPhantomCallbacks();
    void TriggerGCEpilogue();
    void CollectExternalStrings();
    static bool PollForInterrupts(JSContextRef ctx, void* context);
    
    internal::IncrementalMarking incremental_marking_;
    
    v8::internal::GetGlobalHandles getGlobalHandles;
    v8::internal::WeakObjectNearDeath weakObjectNearDeath;
    v8::internal::WeakHeapObjectFinalized weakHeapObjectFinalized;
    v8::internal::WeakJSObjectFinalized weakJSObjectFinalized;
    v8::internal::PerformIncrementalMarking performIncrementalMarking;
};
}} // namespaces

#undef H

#endif /* Isolate_h */
