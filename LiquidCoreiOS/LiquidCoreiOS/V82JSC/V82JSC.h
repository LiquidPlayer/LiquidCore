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
#include <map>
#include <string>

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
    ContextImpl* m_nullContext;
    JSValueRef m_negative_zero;

    void EnterContext(v8::Context *ctx);
    void ExitContext(v8::Context *ctx);
    
    v8::Context *m_external_context;
    
    v8::TryCatch *m_handlers;
    
    std::map<std::string, JSValueRef> m_global_symbols;
    std::map<std::string, JSValueRef> m_private_symbols;
    
    v8::Isolate::CreateParams m_params;
    
    std::stack<ContextImpl*> m_context_stack;
};
}} // namespaces

using v8::internal::IsolateImpl;

struct InternalObjectImpl {
    union {
        unsigned char filler_[v8::internal::Map::kSize];
        v8::internal::Map *pMap;
        v8::internal::Oddball oddball;
        v8::internal::Map map;
    };
    size_t m_slots;
    size_t m_count;
    JSValueRef  m_value;
    IsolateImpl* m_isolate;
};

class HeapAllocator : public v8::internal::MemoryChunk
{
public:
    static InternalObjectImpl* Alloc(IsolateImpl *isolate, size_t size);
};

struct HeapImpl : v8::internal::Heap {
    v8::internal::MemoryChunk *m_heap_top;
    size_t m_index;
    
    static InternalObjectImpl* Alloc(v8::internal::Isolate *isolate, size_t size);
};

struct ContextImpl : InternalObjectImpl
{
    v8::internal::Context *pInternal;
    
    JSContextRef m_ctxRef;
    std::map<std::string, bool> m_loaded_extensions;
    
    static v8::Local<v8::Context> New(v8::Isolate *isolate, JSContextRef ctx);
};

struct ScriptImpl : InternalObjectImpl, v8::Script
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

struct EmbedderDataImpl {
    union {
        struct {
            v8::internal::Map *pMap;
            int m_size;
        };
        uint8_t buffer[v8::internal::Internals::kFixedArrayHeaderSize];
    };
    v8::internal::Object* m_embedder_data[1];
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
struct FunctionTemplateImpl;

struct PropAccessor {
    ValueImpl* name;
    FunctionTemplateImpl *setter;
    FunctionTemplateImpl *getter;
    v8::PropertyAttribute attribute;
    v8::AccessControl settings;
};
struct Prop {
    ValueImpl* name;
    ValueImpl* value;
    v8::PropertyAttribute attributes;
};
struct ObjAccessor {
    ValueImpl *name;
    v8::AccessorNameGetterCallback getter;
    v8::AccessorNameSetterCallback setter;
    ValueImpl *data;
    v8::AccessControl settings;
    v8::PropertyAttribute attribute;
    SignatureImpl *signature;
};

struct LocalException;

struct TemplateImpl : InternalObjectImpl
{
    std::vector<Prop> m_properties;
    std::vector<PropAccessor> m_property_accessors;
    std::vector<ObjAccessor> m_accessors;
    v8::FunctionCallback m_callback;
    JSValueRef m_data;
    SignatureImpl *m_signature;
    ObjectTemplateImpl *m_prototype_template;
    FunctionTemplateImpl *m_parent;

    static JSValueRef callAsFunctionCallback(JSContextRef ctx,
                                             JSObjectRef function,
                                             JSObjectRef thisObject,
                                             size_t argumentCount,
                                             const JSValueRef arguments[],
                                             JSValueRef* exception);
    static v8::MaybeLocal<v8::Object> InitInstance(v8::Local<v8::Context> context,
                                                   JSObjectRef instance,
                                                   LocalException& excep,
                                                   const FunctionTemplateImpl *impl);
    v8::MaybeLocal<v8::Object> InitInstance(v8::Local<v8::Context> context,
                                            JSObjectRef instance, LocalException& exception);
    static TemplateImpl* New(v8::Isolate* isolate, size_t size);
};

struct FunctionTemplateImpl : TemplateImpl
{
    v8::ConstructorBehavior m_behavior;
    std::string m_name;
    int m_length;
    ObjectTemplateImpl *m_instance_template;
    std::map<const ContextImpl*, JSObjectRef> m_functions;
    JSClassRef m_class;

    static JSValueRef callAsConstructorCallback(JSContextRef ctx,
                                                JSObjectRef constructor,
                                                JSObjectRef thisObject,
                                                size_t argumentCount,
                                                const JSValueRef arguments[],
                                                JSValueRef* exception);
};

struct ObjectTemplateImpl : TemplateImpl
{
    FunctionTemplateImpl *m_constructor_template;
    v8::NamedPropertyHandlerConfiguration m_named_handler;
    v8::IndexedPropertyHandlerConfiguration m_indexed_handler;
    JSValueRef m_named_data;
    JSValueRef m_indexed_data;
    bool m_need_proxy;
    int m_internal_fields;
    
    v8::MaybeLocal<v8::Object> NewInstance(v8::Local<v8::Context> context, JSObjectRef root);
};

struct TemplateWrap {
    const TemplateImpl *m_template;
    IsolateImpl* m_isolate;
    std::map<JSStringRef, JSObjectRef> m_getters;
    std::map<JSStringRef, JSObjectRef> m_setters;
};

struct InstanceWrap {
    JSValueRef m_security;
    const ObjectTemplateImpl *m_object_template;
    IsolateImpl *m_isolate;
    int m_num_internal_fields;
    JSValueRef *m_internal_fields;
    JSValueRef m_private_properties;
    int m_hash;
};

struct SignatureImpl
{
    SignatureImpl();
    ~SignatureImpl();

    v8::Isolate *m_isolate;
    FunctionTemplateImpl *m_template;
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
            return JSValueMakeNumber(ToContextRef(context), value);
        } else {
            ValueImpl *that_ = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return that_->m_value;
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
        return ToJSValueRef(v, OperatingContext(isolate));
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(const T* v, v8::Local<v8::Context> context)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(v));
        if (obj->IsSmi()) {
            return JSValueMakeNumber(ToContextRef(context), v8::internal::Smi::ToInt(obj));
        } else {
            ValueImpl *o = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return o->m_value;
        }
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(const T* v, v8::Isolate *isolate)
    {
        return ToJSValueRef(v, OperatingContext(isolate));
    }
    static inline JSContextRef ToContextRef(v8::Local<v8::Context> context)
    {
        return ToContextImpl(context)->m_ctxRef;
    }
    static inline JSContextRef ToContextRef(v8::Isolate *isolate)
    {
        return ToContextImpl(OperatingContext(isolate))->m_ctxRef;
    }
    static inline ContextImpl* ToContextImpl(v8::Local<v8::Context> context)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<v8::Context*>(*context));
        ContextImpl *ctx = reinterpret_cast<ContextImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
        return ctx;
    }
    static inline ContextImpl* ToContextImpl(const v8::Context* thiz)
    {
        return ToContextImpl(_local<v8::Context>(const_cast<v8::Context*>(thiz)).toLocal());
    }
    static inline IsolateImpl* ToIsolateImpl(v8::Isolate *isolate)
    {
        return reinterpret_cast<IsolateImpl*>(isolate);
    }
    static inline v8::Isolate* ToIsolate(IsolateImpl *isolate)
    {
        return reinterpret_cast<v8::Isolate*>(isolate);
    }
    template <class T>
    static inline v8::Isolate* ToIsolate(const T* thiz)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(thiz));
        if (obj->IsSmi()) {
            return v8::Isolate::GetCurrent();
        } else {
            ValueImpl *v = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return ToIsolate(v->m_isolate);
        }
    }
    static inline v8::Local<v8::Context> OperatingContext(v8::Isolate* isolate)
    {
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        if (context.IsEmpty()) {
            context = _local<v8::Context>(ToIsolateImpl(isolate)->m_nullContext).toLocal();
        }
        return context;
    }
    template <class T>
    static inline v8::Local<v8::Context> ToCurrentContext(const T* thiz)
    {
        return OperatingContext(ToIsolate<T>(thiz));
    }
    static inline bool is__(const v8::Value* thiz, const char *name_, const char *code_)
    {
        v8::Local<v8::Context> context = ToCurrentContext(thiz);
        auto ctx = V82JSC::ToContextRef(context);
        auto v = V82JSC::ToJSValueRef<v8::Value>(thiz, context);
        
        JSValueRef b = exec(ctx, code_, 1, &v);
        bool ret = JSValueToBoolean(ctx, b);
        return ret;
    }
    static inline JSValueRef exec(JSContextRef ctx, const char *body, int argc, const JSValueRef *argv, JSValueRef *pexcp=nullptr)
    {
        JSValueRef exception = 0;
        JSStringRef argNames[argc];
        JSStringRef anon = JSStringCreateWithUTF8CString("anon");
        JSStringRef sbody = JSStringCreateWithUTF8CString(body);
        for (int i=0; i<argc; i++) {
            char argname[64];
            sprintf(argname, "_%d", i+1);
            argNames[i] = JSStringCreateWithUTF8CString(argname);
        }
        JSObjectRef function = JSObjectMakeFunction(ctx, anon, argc, argNames, sbody, 0, 0, &exception);
        for (int i=0; i<argc; i++) {
            JSStringRelease(argNames[i]);
        }
        assert(exception==0);

        JSValueRef result = JSObjectCallAsFunction(ctx, function, JSObjectMake(ctx,0,0), argc, argv, &exception);
        if (!pexcp) {
            if (exception!=0) {
                JSStringRef error = JSValueToStringCopy(ctx, exception, 0);
                char msg[JSStringGetMaximumUTF8CStringSize(error)];
                JSStringGetUTF8CString(error, msg, JSStringGetMaximumUTF8CStringSize(error));
                fprintf(stderr, ">>> %s\n", msg);
            }
            assert(exception==0);
        } else {
            *pexcp = exception;
        }
        JSStringRelease(anon);
        JSStringRelease(sbody);
        return result;
    }
#define GLOBAL_PRIVATE_SYMBOL "org.liquidplayer.javascript.__v82jsc_private__"
    static inline InstanceWrap * makePrivateInstance(JSContextRef ctx, JSObjectRef object)
    {
        InstanceWrap *wrap = new InstanceWrap();
        wrap->m_security = object;
        wrap->m_hash = 1 + rand();
        JSValueProtect(ctx, wrap->m_security);
        
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        def.finalize = [](JSObjectRef object) {
            // FIXME: Do something
        };
        JSClassRef klass = JSClassCreate(&def);
        JSObjectRef private_object = JSObjectMake(ctx, klass, (void*)wrap);
        JSClassRelease(klass);
        
        JSValueRef args[] = {
            object, private_object
        };

        exec(ctx,
             "_1[Symbol.for('" GLOBAL_PRIVATE_SYMBOL "')] = _2",
             2, args);
        return wrap;
    }
    static inline InstanceWrap * getPrivateInstance(JSContextRef ctx, JSObjectRef object)
    {
        JSObjectRef private_object = (JSObjectRef) exec(ctx, "return _1[Symbol.for('" GLOBAL_PRIVATE_SYMBOL "')]", 1, &object);
        if (JSValueIsObject(ctx, private_object)) {
            InstanceWrap *wrap = (InstanceWrap*) JSObjectGetPrivate(private_object);
            if (wrap && JSValueIsStrictEqual(ctx, object, wrap->m_security)) {
                return wrap;
            }
        }
        return nullptr;
    }
};
#define IS(name_,code_) V82JSC::is__(this,#name_,code_)

struct LocalException {
    LocalException(IsolateImpl *i) : exception_(0), isolate_(i) {}
    ~LocalException()
    {
        if (isolate_->m_handlers && exception_) {
            reinterpret_cast<TryCatchCopy*>(isolate_->m_handlers)->exception_ = (void*)exception_;
            isolate_->i.ii.thread_local_top()->scheduled_exception_ = reinterpret_cast<v8::internal::Object*>(isolate_->i.roots.the_hole_value);
        }
    }
    inline JSValueRef* operator&()
    {
        return &exception_;
    }
    inline bool ShouldThow() { return exception_ != nullptr; }
    JSValueRef exception_;
    IsolateImpl *isolate_;
};

struct ArrayBufferInfo {
    void *buffer;
    size_t byte_length;
    IsolateImpl *isolate;
    bool isExternal;
    bool isNeuterable;
};

ArrayBufferInfo * GetArrayBufferInfo(const v8::ArrayBuffer *ab);

struct ArrayBufferViewInfo {
};

ArrayBufferViewInfo * GetArrayBufferViewInfo(const v8::ArrayBufferView *abv);
void proxyArrayBuffer(ContextImpl *ctx);
bool InstallAutoExtensions(v8::Local<v8::Context> context);
bool InstallExtension(v8::Local<v8::Context> context, const char *extension_name);

#endif /* V82JSC_h */
