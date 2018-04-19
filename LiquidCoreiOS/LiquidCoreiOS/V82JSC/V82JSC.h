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

#define MAX_HANDLES_PER_GROUP 32
struct HandleGroup {
    v8::internal::Object * handles_[MAX_HANDLES_PER_GROUP];
    HandleGroup *next_;
};
    
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
    ContextImpl* m_defaultContext;

    void EnterContext(v8::Context *ctx);
    void ExitContext(v8::Context *ctx);
    
    v8::Context *current_context;
    v8::Context *m_external_context;
    
    v8::TryCatch *m_handlers;
    std::vector<HandleGroup> m_handles;
    int m_handle_index;
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
    ContextImpl* m_context;
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
struct FunctionTemplateImpl;

struct PropAccessor {
    FunctionTemplateImpl *m_setter;
    FunctionTemplateImpl *m_getter;
};
struct ObjAccessor {
    JSValueRef m_property;
    v8::AccessorSetterCallback m_setter;
    v8::AccessorGetterCallback m_getter;
    JSValueRef m_data;
    const ContextImpl* m_context;
};

struct LocalException;

struct TemplateImpl : InternalObjectImpl
{
    v8::Isolate *m_isolate;
    std::map<JSStringRef, ValueImpl*> m_properties;
    std::map<JSStringRef, PropAccessor> m_property_accessors;
    std::map<JSStringRef, ObjAccessor> m_obj_accessors;
    v8::FunctionCallback m_callback;
    JSValueRef m_data;
    SignatureImpl *m_signature;
    ObjectTemplateImpl *m_prototype_template;
    JSClassDefinition m_definition;
    FunctionTemplateImpl *m_parent;

    static JSValueRef callAsFunctionCallback(JSContextRef ctx,
                                             JSObjectRef function,
                                             JSObjectRef thisObject,
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
    
    v8::MaybeLocal<v8::Object> NewInstance(v8::Local<v8::Context> context, JSObjectRef root);
};

struct TemplateWrap {
    const TemplateImpl *m_template;
    const ContextImpl* m_context;
    std::map<JSStringRef, JSObjectRef> m_getters;
    std::map<JSStringRef, JSObjectRef> m_setters;
};

struct InstanceWrap {
    const ObjectTemplateImpl *m_object_template;
    const ContextImpl *m_context;
};

struct SignatureImpl
{
    SignatureImpl();
    ~SignatureImpl();

    v8::Isolate *m_isolate;
    FunctionTemplateImpl *m_template;
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
    static inline JSValueRef exec(JSContextRef ctx, const char *body, int argc, const JSValueRef *argv)
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
        assert(exception==0);
        JSValueRef result = JSObjectCallAsFunction(ctx, function, 0, argc, argv, &exception);
        assert(exception==0);
        for (int i=0; i<argc; i++) {
            JSStringRelease(argNames[i]);
        }
        JSStringRelease(anon);
        JSStringRelease(sbody);
        return result;
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
