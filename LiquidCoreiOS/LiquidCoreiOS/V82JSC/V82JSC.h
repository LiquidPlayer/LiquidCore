//
//  V82JSC.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 3/18/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef V82JSC_h
#define V82JSC_h

#include <JavaScriptCore/JavaScript.h>
#include "include/v8-util.h"
#include "src/arguments.h"
#include "src/base/platform/platform.h"
#include "src/code-stubs.h"
#include "src/compilation-cache.h"
#include "src/debug/debug.h"
#include "src/execution.h"
#include "src/futex-emulation.h"
#include "src/heap/incremental-marking.h"
#include "src/api.h"
#include "src/lookup.h"
#include "src/objects-inl.h"
#include "src/parsing/preparse-data.h"
#include "src/profiler/cpu-profiler.h"
#include "src/unicode-inl.h"
#include "src/utils.h"
#include "src/vm-state.h"
#include "src/heap/heap.h"

#define DEF(T,V,F) \
v8::internal::Object ** V;
struct Roots {
    STRONG_ROOT_LIST(DEF)
};

template <class T>
class _maybe {
public:
    bool has_value_;
    T value_;
    inline v8::Maybe<T> toMaybe() { return *(reinterpret_cast<v8::Maybe<T> *>(this)); }
    explicit _maybe(const T& t) : has_value_(true), value_(t) {}
    _maybe() : has_value_(false) {}
};

template <class T>
class _local {
public:
    T* val_;
    v8::Local<T> toLocal() { return *(reinterpret_cast<v8::Local<T> *>(this)); }
    _local(void *v) { val_ = reinterpret_cast<T*>(v); }
};

struct ContextImpl;

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

struct IsolateImpl {
    union i_ {
        v8::internal::Isolate ii;
        /*
        struct internal_copy_ {
            v8::base::Atomic32 id_;
            void* entry_stack_; //EntryStackItem* entry_stack_;
            int stack_trace_nesting_level_;
            StringStream* incomplete_message_;
            Address isolate_addresses_[kIsolateAddressCount + 1];  // NOLINT
            Bootstrapper* bootstrapper_;
            RuntimeProfiler* runtime_profiler_;
            CompilationCache* compilation_cache_;
            std::shared_ptr<Counters> async_counters_;
            v8::base::RecursiveMutex break_access_;
            Logger* logger_;
            StackGuard stack_guard_;
            StubCache* load_stub_cache_;
            StubCache* store_stub_cache_;
            CodeAgingHelper* code_aging_helper_;
            DeoptimizerData* deoptimizer_data_;
            bool deoptimizer_lazy_throw_;
            MaterializedObjectStore* materialized_object_store_;
            ThreadLocalTop thread_local_top_;
            bool capture_stack_trace_for_uncaught_exceptions_;
            int stack_trace_for_uncaught_exceptions_frame_limit_;
            v8::StackTrace::StackTraceOptions stack_trace_for_uncaught_exceptions_options_;
            ContextSlotCache* context_slot_cache_;
            DescriptorLookupCache* descriptor_lookup_cache_;
            HandleScopeData handle_scope_data_;
            HandleScopeImplementer* handle_scope_implementer_;
            UnicodeCache* unicode_cache_;
            AccountingAllocator* allocator_;
            InnerPointerToCodeCache* inner_pointer_to_code_cache_;
            GlobalHandles* global_handles_;
            EternalHandles* eternal_handles_;
            ThreadManager* thread_manager_;
            RuntimeState runtime_state_;
            Builtins builtins_;
            SetupIsolateDelegate* setup_delegate_;
            unibrow::Mapping<unibrow::Ecma262UnCanonicalize> jsregexp_uncanonicalize_;
            unibrow::Mapping<unibrow::CanonicalizationRange> jsregexp_canonrange_;
            unibrow::Mapping<unibrow::Ecma262Canonicalize>
            regexp_macro_assembler_canonicalize_;
            RegExpStack* regexp_stack_;
            List<int> regexp_indices_;
            DateCache* date_cache_;
            CallInterfaceDescriptorData* call_descriptor_data_;
            AccessCompilerData* access_compiler_data_;
            v8::base::RandomNumberGenerator* random_number_generator_;
            v8::base::AtomicValue<v8::RAILMode> rail_mode_;
            bool promise_hook_or_debug_is_active_;
            v8::PromiseHook promise_hook_;
            v8::HostImportModuleDynamicallyCallback host_import_module_dynamically_callback_;
            v8::base::Mutex rail_mutex_;
            double load_start_time_ms_;
            
            // Whether the isolate has been created for snapshotting.
            bool serializer_enabled_;
            
            // True if fatal error has been signaled for this isolate.
            bool has_fatal_error_;
            
            // True if this isolate was initialized from a snapshot.
            bool initialized_from_snapshot_;
            
            // True if ES2015 tail call elimination feature is enabled.
            bool is_tail_call_elimination_enabled_;
            
            // True if the isolate is in background. This flag is used
            // to prioritize between memory usage and latency.
            bool is_isolate_in_background_;
            
            // Time stamp at initialization.
            double time_millis_at_init_;
            
#ifdef DEBUG
            // A static array of histogram info for each type.
            HistogramInfo heap_histograms_[LAST_TYPE + 1];
            JSObject::SpillInformation js_spill_information_;
#endif
            
            v8::internal::Debug* debug_;
            v8::internal::CpuProfiler* cpu_profiler_;
            v8::internal::HeapProfiler* heap_profiler_;
            std::unique_ptr<CodeEventDispatcher> code_event_dispatcher_;
            v8::FunctionEntryHook function_entry_hook_;
            
            const AstStringConstants* ast_string_constants_;
            
            interpreter::Interpreter* interpreter_;
            
            CompilerDispatcher* compiler_dispatcher_;
            
            typedef std::pair<InterruptCallback, void*> InterruptEntry;
            std::queue<InterruptEntry> api_interrupts_queue_;
#define debug v8::debug
#define GLOBAL_BACKING_STORE(type, name, initialvalue)                         \
type name##_;
            ISOLATE_INIT_LIST(GLOBAL_BACKING_STORE)
#undef GLOBAL_BACKING_STORE
#undef debug
            
#define GLOBAL_ARRAY_BACKING_STORE(type, name, length)                         \
type name##_[length];
            ISOLATE_INIT_ARRAY_LIST(GLOBAL_ARRAY_BACKING_STORE)
#undef GLOBAL_ARRAY_BACKING_STORE
            
#ifdef DEBUG
            // This class is huge and has a number of fields controlled by
            // preprocessor defines. Make sure the offsets of these fields agree
            // between compilation units.
#define ISOLATE_FIELD_OFFSET(type, name, ignored)                              \
static const intptr_t name##_debug_offset_;
            ISOLATE_INIT_LIST(ISOLATE_FIELD_OFFSET)
            ISOLATE_INIT_ARRAY_LIST(ISOLATE_FIELD_OFFSET)
#undef ISOLATE_FIELD_OFFSET
#endif
            
            DeferredHandles* deferred_handles_head_;
            OptimizingCompileDispatcher* optimizing_compile_dispatcher_;
            
            // Counts deopt points if deopt_every_n_times is enabled.
            unsigned int stress_deopt_count_;
            
            int next_optimization_id_;
            
#if V8_SFI_HAS_UNIQUE_ID
            int next_unique_sfi_id_;
#endif
            
            // List of callbacks before a Call starts execution.
            List<BeforeCallEnteredCallback> before_call_entered_callbacks_;
            
            // List of callbacks when a Call completes.
            List<CallCompletedCallback> call_completed_callbacks_;
            
            // List of callbacks after microtasks were run.
            List<MicrotasksCompletedCallback> microtasks_completed_callbacks_;
            bool is_running_microtasks_;
            
            v8::Isolate::UseCounterCallback use_counter_callback_;
            BasicBlockProfiler* basic_block_profiler_;
            
            List<Object*> partial_snapshot_cache_;
            
            v8::ArrayBuffer::Allocator* array_buffer_allocator_;
            
            FutexWaitListNode futex_wait_list_node_;
            
            CancelableTaskManager* cancelable_task_manager_;
            
            std::unique_ptr<wasm::CompilationManager> wasm_compilation_manager_;
            
            v8::debug::ConsoleDelegate* console_delegate_ = nullptr;
            
            v8::Isolate::AbortOnUncaughtExceptionCallback
            abort_on_uncaught_exception_callback_;
            
#ifdef USE_SIMULATOR
            base::Mutex simulator_i_cache_mutex_;
            base::Mutex simulator_redirection_mutex_;
#endif
            
            bool allow_atomics_wait_;
            
            v8::internal::Isolate::ManagedObjectFinalizer managed_object_finalizers_list_;
            
            size_t total_regexp_code_generated_;
            
            size_t elements_deletion_counter_ = 0;
            
            // The top entry of the v8::Context::BackupIncumbentScope stack.
            const v8::Context::BackupIncumbentScope* top_backup_incumbent_scope_ =
            nullptr;
        } internal_copy;
        */
        struct {
            void *i0; // kHeapObjectMapOffset, kIsolateEmbedderDataOffset
            void *i1; // kForeignAddressOffset
            void *i2;
            void *i3;
            uint64_t i64_0; // kExternalMemoryOffset
            uint64_t i64_1; // kExternalMemoryLimitOffset
            uint64_t i64_2;
            void *i4;
            void *i5;
            struct Roots roots; // kIsolateRootsOffset
        };
    } i;
    JSContextGroupRef m_group;
    ContextImpl* m_defaultContext;

    void EnterContext(v8::Context *ctx);
    void ExitContext(v8::Context *ctx);
    
    v8::Context *current_context;
    v8::Context *m_external_context;
    
    v8::TryCatch *m_handlers;
};
}} // namespaces

using v8::internal::IsolateImpl;

struct ContextImpl;

struct InternalObjectImpl {
    union {
        v8::internal::Map *pMap;
        v8::internal::Oddball oddball;
        v8::internal::Map map;
        unsigned char filler_[256]; // FIXME
    };
    JSValueRef  m_value;
    JSStringRef m_string;
    JSObjectRef m_object;
    ContextImpl* m_context;
    double m_number;
};

struct ContextImpl : v8::Context
{
    v8::internal::Context *pInternal;
    
    JSContextRef m_context;
    IsolateImpl *isolate;
    
    typedef enum _IsFunctions {
        IsFunction,
        IsSymbol,
        IsArgumentsObject,
        IsBooleanObject,
        IsNumberObject,
        IsStringObject,
        IsSymbolObject,
        IsNativeError,
        IsRegExp,
        IsAsyncFunction,
        IsGeneratorFunction,
        IsGeneratorObject,
        IsPromise,
        IsMap,
        IsSet,
        IsMapIterator,
        IsSetIterator,
        IsWeakMap,
        IsWeakSet,
        IsArrayBuffer,
        IsArrayBufferView,
        IsTypedArray,
        IsUint8Array,
        IsUint8ClampedArray,
        IsInt8Array,
        IsUint16Array,
        IsInt16Array,
        IsUint32Array,
        IsInt32Array,
        IsFloat32Array,
        IsFloat64Array,
        IsDataView,
        IsSharedArrayBuffer,
        IsProxy,
        IsExternal,
        
        TypeOf,
        InstanceOf,
        ValueOf,
        
        NewBooleanObject,
        NewStringObject,
        NewSymbolObject,
        NewNumberObject,
        
        SIZE
    } IsFunctions;
    JSObjectRef IsFunctionRefs[IsFunctions::SIZE];
};

struct ArrayBufferViewImpl : v8::ArrayBufferView
{
    int m_byte_length;
};

struct ScriptImpl : v8::Script
{
    JSStringRef m_sourceURL;
    int m_startingLineNumber;
    JSStringRef m_script;
};

struct ValueImpl : InternalObjectImpl, v8::Value {
    static v8::Local<Value> New(const ContextImpl *ctx, JSValueRef value);
    static v8::Local<v8::String> New(v8::Isolate *isolate, JSStringRef string,
                                     v8::internal::InstanceType type=v8::internal::FIRST_NONSTRING_TYPE,
                                     void *resource = nullptr);
    static v8::Primitive * NewUndefined(v8::Isolate *isolate);
    static v8::Primitive * New(v8::Isolate *isolate, double number);
    static v8::Primitive * NewBoolean(v8::Isolate *isolate, bool value);
    static v8::Primitive * NewNull(v8::Isolate *isolate);
};

struct FunctionCallbackImpl : public v8::FunctionCallbackInfo<v8::Value>
{
    inline FunctionCallbackImpl(v8::internal::Object** implicit_args,
                                v8::internal::Object** values, int length) :
    FunctionCallbackInfo<v8::Value>(implicit_args, values, length) {}
};

template <class T>
struct PropertyCallbackImpl : public v8::PropertyCallbackInfo<T>
{
    inline PropertyCallbackImpl(v8::internal::Object** args) :
    v8::PropertyCallbackInfo<T>(args) {}
};

struct SignatureImpl;
struct ObjectTemplateImpl;

struct PropAccessor {
    ObjectTemplateImpl *m_setter;
    ObjectTemplateImpl *m_getter;
};
struct ObjAccessor {
    JSValueRef m_property;
    v8::AccessorSetterCallback m_setter;
    v8::AccessorGetterCallback m_getter;
    JSValueRef m_data;
    const ContextImpl* m_context;
};

struct ObjectTemplateImpl : ValueImpl
{
    v8::FunctionCallback m_callback;
    v8::Isolate *m_isolate;
    SignatureImpl *m_signature;
    v8::ConstructorBehavior m_behavior;
    JSClassRef m_class;
    JSClassDefinition m_definition;
    std::map<JSStringRef, ValueImpl*> m_properties;
    std::map<JSStringRef, PropAccessor> m_property_accessors;
    std::map<JSStringRef, ObjAccessor> m_obj_accessors;
    ObjectTemplateImpl *m_parent;
    JSValueRef m_data;
    std::string m_name;
    int m_length;
    ObjectTemplateImpl *m_constructor_template;
    JSObjectRef m_function;
    
    static JSValueRef callAsFunctionCallback(JSContextRef ctx,
                                             JSObjectRef function,
                                             JSObjectRef thisObject,
                                             size_t argumentCount,
                                             const JSValueRef arguments[],
                                             JSValueRef* exception);
    static JSObjectRef callAsConstructorCallback(JSContextRef ctx,
                                                 JSObjectRef constructor,
                                                 size_t argumentCount,
                                                 const JSValueRef arguments[],
                                                 JSValueRef* exception);
    static JSValueRef objectGetterCallback(JSContextRef ctx,
                                           JSObjectRef ignore,
                                           JSObjectRef thisObject,
                                           size_t argumentCount,
                                           const JSValueRef arguments[],
                                           JSValueRef* exception);
    static JSValueRef objectSetterCallback(JSContextRef ctx,
                                           JSObjectRef ignore,
                                           JSObjectRef thisObject,
                                           size_t argumentCount,
                                           const JSValueRef arguments[],
                                           JSValueRef* exception);
};

struct ObjectTemplateWrap {
    const ObjectTemplateImpl *m_template;
    const ContextImpl* m_context;
    std::map<JSStringRef, JSObjectRef> m_getters;
    std::map<JSStringRef, JSObjectRef> m_setters;
};

struct SignatureImpl
{
    SignatureImpl();
    ~SignatureImpl();

    v8::Isolate *m_isolate;
    ObjectTemplateImpl *m_template;
};

struct TypedArrayImpl : public ArrayBufferViewImpl, v8::TypedArray {
};

struct Uint8ClampedArrayImpl : public TypedArrayImpl, v8::Uint8ClampedArray {
};

struct Uint8ArrayImpl : public TypedArrayImpl, v8::Uint8Array {
};

struct Uint16ArrayImpl : public TypedArrayImpl, v8::Uint16Array {
};

struct Uint32ArrayImpl : public TypedArrayImpl, v8::Uint32Array {
};

struct Int8ArrayImpl : public TypedArrayImpl, v8::Int8Array {
};

struct Int16ArrayImpl : public TypedArrayImpl, v8::Int16Array {
};

struct Int32ArrayImpl : public TypedArrayImpl, v8::Int32Array {
};

struct Float32ArrayImpl : public TypedArrayImpl, v8::Float32Array {
};

struct Float64ArrayImpl : public TypedArrayImpl, v8::Float64Array {
};

/* IMPORTANT!  This must match v8::TryCatch */
struct TryCatchCopy {
    v8::internal::Isolate* isolate_;
    v8::TryCatch* next_;
    void* exception_;
    void* message_obj_;
    void* js_stack_comparable_address_;
    bool is_verbose_ : 1;
    bool can_continue_ : 1;
    bool capture_message_ : 1;
    bool rethrow_ : 1;
    bool has_terminated_ : 1;
};

struct V82JSC {
    template <class T>
    static inline JSValueRef ToJSValueRef(v8::Local<T> v, v8::Local<v8::Context> context)
    {
        if (v.IsEmpty()) return 0;
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(*v);
        if (obj->IsSmi()) {
            int value = v8::internal::Smi::ToInt(obj);
            return JSValueMakeNumber(reinterpret_cast<ContextImpl*>(*context)->m_context, value);
        } else {
            ValueImpl *that_ = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return that_->m_value;
        }
    }
    template <class T>
    static inline ContextImpl* ToContextImpl(const T* v)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(v));
        if (obj->IsSmi()) {
            return nullptr;
        } else {
            ValueImpl *that_ = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return that_->m_context;
        }
    }
    template <class I, class T>
    static inline I* ToImpl(v8::Local<T> v)
    {
        if (v.IsEmpty()) return nullptr;
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(*v);
        if (!obj->IsSmi()) {
            I *that_ = reinterpret_cast<I*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return that_;
        }
        return nullptr;
    }
    template <class I, class T>
    static inline I* ToImpl(const T* thiz)
    {
        return ToImpl<I>(_local<T>(const_cast<T*>(thiz)).toLocal());
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(v8::Local<T> v, v8::Isolate *isolate)
    {
        return ToJSValueRef(v, _local<v8::Context>(reinterpret_cast<IsolateImpl*>(isolate)->m_defaultContext).toLocal());
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(const T* v, v8::Local<v8::Context> context)
    {
        return ToJSValueRef(_local<T>(const_cast<T*>(v)).toLocal(), context);
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(const T* v, v8::Isolate *isolate)
    {
        return ToJSValueRef(_local<T>(const_cast<T*>(v)).toLocal(),
                            _local<v8::Context>(reinterpret_cast<IsolateImpl*>(isolate)->m_defaultContext).toLocal());
    }
    static inline JSContextRef ToContextRef(v8::Local<v8::Context> context)
    {
        ContextImpl *ctx = ToContextImpl(context);
        return ctx->m_context;
    }
    static inline JSContextRef ToContextRef(v8::Isolate *isolate)
    {
        IsolateImpl *impl = reinterpret_cast<IsolateImpl*>(isolate);
        return impl->m_defaultContext->m_context;
    }
    static inline ContextImpl* ToContextImpl(v8::Local<v8::Context> context)
    {
        ContextImpl *ctx = * reinterpret_cast<ContextImpl**>(*context);
        return ctx;
    }
    static inline ContextImpl* ToContextImpl(const v8::Context* thiz)
    {
        ContextImpl *ctx = * reinterpret_cast<ContextImpl**>(const_cast<v8::Context*>(thiz));
        return ctx;
    }
    static inline IsolateImpl* ToIsolateImpl(v8::Isolate *isolate)
    {
        return reinterpret_cast<IsolateImpl*>(isolate);
    }
    static inline v8::Isolate* ToIsolate(IsolateImpl *isolate)
    {
        return reinterpret_cast<v8::Isolate*>(isolate);
    }
    static inline JSObjectRef jsfunc__(const char *name_, const char *code_, ContextImpl *c, int index)
    {
        if (!c->IsFunctionRefs[index]) {
            JSStringRef name = JSStringCreateWithUTF8CString(name_);
            JSStringRef param = JSStringCreateWithUTF8CString("v");
            JSStringRef body = JSStringCreateWithUTF8CString(code_);
            JSValueRef exception = nullptr;
            c->IsFunctionRefs[index] = JSObjectMakeFunction(c->m_context, name, 1, &param, body, 0, 0, &exception);
            JSValueProtect(c->m_context, c->IsFunctionRefs[index]);
            assert(exception==nullptr);
            JSStringRelease(name);
            JSStringRelease(param);
            JSStringRelease(body);
        }
        return c->IsFunctionRefs[index];
    }
    
    static inline bool is__(const v8::Value* thiz, const char *name_, const char *code_, int index)
    {
        auto c = V82JSC::ToContextImpl(thiz);
        auto v = V82JSC::ToJSValueRef<v8::Value>(thiz, _local<v8::Context>(c).toLocal());
        JSValueRef exception = nullptr;
        bool ret = JSValueToBoolean(c->m_context,
                                    JSObjectCallAsFunction(c->m_context, jsfunc__(name_, code_, c, index), 0, 1, &v, &exception));
        assert(exception==nullptr);
        return ret;
    }
};
#define JSFUNC(name_,code_,c) V82JSC::jsfunc__(#name_,code_,c,ContextImpl::IsFunctions::name_)
#define IS(name_,code_) V82JSC::is__(this,#name_,code_,ContextImpl::IsFunctions::name_)

struct LocalException {
    LocalException(IsolateImpl *i) : exception_(0), isolate_(i) {}
    ~LocalException()
    {
        if (isolate_->m_handlers && exception_) {
            reinterpret_cast<TryCatchCopy*>(isolate_->m_handlers)->exception_ = (void*)exception_;
        }
    }
    inline JSValueRef* operator&()
    {
        return &exception_;
    }
    inline bool ShouldThow() { return exception_ != nullptr; }
private:
    JSValueRef exception_;
    IsolateImpl *isolate_;
};

#endif /* V82JSC_h */
