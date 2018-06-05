//
//  V82JSC.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 3/18/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef V82JSC_h
#define V82JSC_h

#include "Isolate.h"

template <class T>
class _maybe {
public:
    bool has_value_;
    T value_;
    inline v8::Maybe<T> toMaybe() { return *(reinterpret_cast<v8::Maybe<T> *>(this)); }
    explicit _maybe(const T& t) : has_value_(true), value_(t) {}
    _maybe() : has_value_(false) {}
};

#define Copyable(T) \
v8::Persistent<T, v8::CopyablePersistentTraits<T>>

struct ContextImpl;

using v8::internal::IsolateImpl;

struct ContextImpl : V82JSC_HeapObject::Context
{
    static v8::Local<v8::Context> New(v8::Isolate *isolate, JSContextRef ctx);
};

struct ScriptImpl : V82JSC_HeapObject::Script {};

struct ValueImpl : V82JSC_HeapObject::Value
{
    static v8::Local<v8::Value> New(const ContextImpl *ctx, JSValueRef value, V82JSC_HeapObject::BaseMap *map=nullptr);
};

struct StringImpl : V82JSC_HeapObject::String
{
    static v8::Local<v8::String> New(v8::Isolate *isolate, JSStringRef string,
                                     V82JSC_HeapObject::BaseMap *map=nullptr,
                                     void *resource = nullptr);
};

struct HiddenObjectImpl : ValueImpl {
    void PropagateOwnPropertyToChild(v8::Local<v8::Context> context, v8::Local<v8::Name> property, JSObjectRef child);
    void PropagateOwnPropertyToChildren(v8::Local<v8::Context> context, v8::Local<v8::Name> property);
    void PropagateOwnPropertiesToChild(v8::Local<v8::Context> context, JSObjectRef child);
};

struct EmbedderDataImpl : V82JSC_HeapObject::FixedArray {};

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

struct PropAccessorImpl : V82JSC_HeapObject::PropAccessor {};
struct PropImpl : V82JSC_HeapObject::Prop {};
struct ObjAccessorImpl : V82JSC_HeapObject::ObjAccessor {};
struct IntrinsicPropImpl : V82JSC_HeapObject::IntrinsicProp {};
struct TrackedObjectImpl : V82JSC_HeapObject::TrackedObject {};
struct AccessorImpl : V82JSC_HeapObject::Accessor {};

struct LocalException;

struct TemplateImpl : V82JSC_HeapObject::Template
{
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
    static v8::MaybeLocal<v8::Object> InitInstance(v8::Local<v8::Context> context,
                                                   JSObjectRef instance,
                                                   LocalException& excep,
                                                   v8::Local<v8::FunctionTemplate> ftempl);
    v8::MaybeLocal<v8::Object> InitInstance(v8::Local<v8::Context> context,
                                            JSObjectRef instance, LocalException& exception);
    static TemplateImpl* New(v8::Isolate* isolate, size_t size);
};

struct FunctionTemplateImpl : V82JSC_HeapObject::FunctionTemplate
{
    static JSValueRef callAsConstructorCallback(JSContextRef ctx,
                                                JSObjectRef constructor,
                                                JSObjectRef thisObject,
                                                size_t argumentCount,
                                                const JSValueRef arguments[],
                                                JSValueRef* exception);
};

struct ObjectTemplateImpl : V82JSC_HeapObject::ObjectTemplate
{
    v8::MaybeLocal<v8::Object> NewInstance(v8::Local<v8::Context> context, JSObjectRef root, bool isHiddenPrototype);
};

struct SignatureImpl : V82JSC_HeapObject::Signature {};

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
    class __local {
    public:
        T* val_;
        v8::Local<T> toLocal() { return *(reinterpret_cast<v8::Local<T> *>(this)); }
        __local(void *v) { val_ = reinterpret_cast<T*>(v); }
    };

    template <class T>
    static inline v8::Local<T> CreateLocal(v8::internal::Isolate *isolate, V82JSC_HeapObject::HeapObject *o)
    {
        v8::internal::Object ** handle =
            v8::internal::HandleScope::CreateHandle(isolate,
                reinterpret_cast<v8::internal::Object*>(reinterpret_cast<intptr_t>(o) + v8::internal::kHeapObjectTag));
        return __local<T>(handle).toLocal();
    }
    template <class T>
    static inline v8::Local<T> CreateLocal(v8::Isolate *isolate, V82JSC_HeapObject::HeapObject *o)
    {
        return CreateLocal<T>(reinterpret_cast<v8::internal::Isolate*>(isolate), o);
    }
    template <class T>
    static inline v8::Local<T> CreateLocalSmi(v8::internal::Isolate *isolate, v8::internal::Smi* smi)
    {
        v8::internal::Object ** handle = v8::internal::HandleScope::CreateHandle(isolate, smi);
        return __local<T>(handle).toLocal();
    }
    
    template<class T>
    static inline JSValueRef ToJSValueRef_(v8::internal::Object *obj, v8::Local<v8::Context> context)
    {
        JSContextRef ctx = ToContextRef(context);
        if (obj->IsSmi()) {
            int value = v8::internal::Smi::ToInt(obj);
            return JSValueMakeNumber(ctx, value);
        } else {
            auto *that_ = reinterpret_cast<V82JSC_HeapObject::HeapObject*>
                (reinterpret_cast<intptr_t>(obj) - v8::internal::kHeapObjectTag);
            if (that_->m_map == obj) {
                // We are a map, which means this is an oddball value
                IsolateImpl *i = ToIsolateImpl(that_);
                typedef v8::internal::Heap::RootListIndex R;
                if (that_->m_map == i->ii.heap()->root(R::kUndefinedValueRootIndex)) return JSValueMakeUndefined(ctx);
                if (that_->m_map == i->ii.heap()->root(R::kNullValueRootIndex)) return JSValueMakeNull(ctx);
                if (that_->m_map == i->ii.heap()->root(R::kTrueValueRootIndex)) return JSValueMakeBoolean(ctx, true);
                if (that_->m_map == i->ii.heap()->root(R::kFalseValueRootIndex)) return JSValueMakeBoolean(ctx, false);
                if (that_->m_map == i->ii.heap()->root(R::kempty_stringRootIndex)) return i->m_empty_string;
                assert(0);
            }
            return reinterpret_cast<ValueImpl*>(that_)->m_value;
        }
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(v8::Local<T> v, v8::Local<v8::Context> context)
    {
        if (v.IsEmpty()) return 0;
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(*v);
        return ToJSValueRef_<T>(obj, context);
    }
    template<class T>
    static inline JSValueRef ToJSValueRef(const T* v, v8::Local<v8::Context> context)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(v));
        return ToJSValueRef_<T>(obj, context);
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(v8::Local<T> v, v8::Isolate *isolate)
    {
        return ToJSValueRef(v, OperatingContext(isolate));
    }
    template <class T>
    static inline JSValueRef ToJSValueRef(const T* v, v8::Isolate *isolate)
    {
        return ToJSValueRef(v, OperatingContext(isolate));
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
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(thiz));
        if (!obj->IsSmi()) {
            I *that_ = reinterpret_cast<I*>(reinterpret_cast<intptr_t>(obj) & ~3);
            return that_;
        }
        return nullptr;
    }
    
    static inline ContextImpl* ToContextImpl(v8::Local<v8::Context> context)
    {
        return ToImpl<ContextImpl, v8::Context>(context);
    }
    static inline ContextImpl* ToContextImpl(const v8::Context* thiz)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<v8::Context*>(thiz));
        assert(!obj->IsSmi());
        ContextImpl *ctx = reinterpret_cast<ContextImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
        return ctx;
    }

    static inline JSContextRef ToContextRef(v8::Local<v8::Context> context)
    {
        return ToContextImpl(context)->m_ctxRef;
    }
    static inline JSContextRef ToContextRef(v8::Isolate *isolate)
    {
        return ToContextImpl(OperatingContext(isolate))->m_ctxRef;
    }

    static inline IsolateImpl* ToIsolateImpl(v8::Isolate *isolate)
    {
        return reinterpret_cast<IsolateImpl*>(isolate);
    }
    static inline IsolateImpl* ToIsolateImpl(const V82JSC_HeapObject::HeapObject *io)
    {
        v8::internal::Isolate* isolate =
            reinterpret_cast<v8::internal::HeapObject*>
            (V82JSC_HeapObject::ToHeapPointer(const_cast<V82JSC_HeapObject::HeapObject *>(io)))->GetIsolate();
        return reinterpret_cast<IsolateImpl*>(isolate);
    }
    static inline v8::Isolate* ToIsolate(IsolateImpl *isolate)
    {
        return reinterpret_cast<v8::Isolate*>(isolate);
    }
    static inline IsolateImpl* ToIsolateImpl(const v8::Value *val)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<v8::Value*>(val));
        assert(!obj->IsSmi());
        v8::internal::Isolate* isolate = reinterpret_cast<v8::internal::HeapObject*>(obj)->GetIsolate();
        return reinterpret_cast<IsolateImpl*>(isolate);
    }
    
    template <class T>
    static inline v8::Isolate* ToIsolate(const T* thiz)
    {
        v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(thiz));
        if (obj->IsSmi()) {
            return v8::Isolate::GetCurrent();
        } else {
            v8::internal::Isolate *i = reinterpret_cast<v8::internal::HeapObject*>(obj)->GetIsolate();
            return reinterpret_cast<v8::Isolate*>(i);
        }
    }
    
    static inline v8::Local<v8::Context> OperatingContext(v8::Isolate* isolate)
    {
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        if (context.IsEmpty()) {
            context = v8::Local<v8::Context>::New(isolate, ToIsolateImpl(isolate)->m_nullContext);
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
    static JSObjectRef make_exec_function(JSGlobalContextRef ctx, const char *body, int argc);

    static inline JSValueRef exec(JSContextRef ctx, const char *body, int argc,
                                  const JSValueRef *argv, JSValueRef *pexcp=nullptr)
    {
        JSGlobalContextRef gctx = JSContextGetGlobalContext(ctx);
        IsolateImpl* iso = IsolateImpl::s_context_to_isolate_map[gctx];
        JSObjectRef function = iso->m_exec_maps[gctx].count(body) == 0 ?
            make_exec_function(gctx, body, argc) :
            iso->m_exec_maps[gctx][body];

        JSValueRef exception = 0;
        JSValueRef result = JSObjectCallAsFunction(ctx, function, 0, argc, argv, &exception);
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
        return result;
    }
    static inline v8::Local<v8::Context> FindGlobalContext(v8::Local<v8::Context> context)
    {
        v8::Isolate* isolate = ToIsolate(V82JSC::ToContextImpl(context));
        IsolateImpl *i = ToIsolateImpl(isolate);
        JSGlobalContextRef gctx = JSContextGetGlobalContext(V82JSC::ToContextRef(context));
        if (i->m_global_contexts.count(gctx) > 0) {
            return i->m_global_contexts[gctx].Get(isolate);
        }
        return v8::Local<v8::Context>();
    }
    static inline JSValueRef GetRealPrototype(v8::Local<v8::Context> context, JSObjectRef obj)
    {
        v8::Isolate* isolate = ToIsolate(V82JSC::ToContextImpl(context));
        v8::Local<v8::Context> global_context = FindGlobalContext(context);
        if (global_context.IsEmpty()) {
            // No worries, it just means this hasn't been set up yet; use the native API
            return JSObjectGetPrototype(ToContextRef(context), obj);
        }
        v8::Local<v8::Function> getPrototype = ToContextImpl(global_context)->ObjectGetPrototypeOf.Get(isolate);
        CHECK(!getPrototype.IsEmpty());
        v8::Local<v8::Value> args[] = {
            v8::Local<v8::Value>(ValueImpl::New(ToContextImpl(context), obj))
        };
        v8::Local<v8::Value> our_proto_local = getPrototype->Call(context, Null(isolate), 1, args).ToLocalChecked();
        JSValueRef our_proto = ToJSValueRef(our_proto_local, context);
        return our_proto;
    }
    static inline void SetRealPrototype(v8::Local<v8::Context> context, JSObjectRef obj, JSValueRef proto)
    {
        v8::Isolate* isolate = ToIsolate(V82JSC::ToContextImpl(context));
        v8::Local<v8::Context> global_context = FindGlobalContext(context);
        if (global_context.IsEmpty()) {
            // No worries, it just means this hasn't been set up yet; use the native API
            JSObjectSetPrototype(ToContextRef(context), obj, proto);
            return;
        }
        v8::Local<v8::Function> setPrototype = ToContextImpl(global_context)->ObjectSetPrototypeOf.Get(isolate);
        CHECK(!setPrototype.IsEmpty());
        v8::Local<v8::Value> args[] = {
            v8::Local<v8::Value>(ValueImpl::New(ToContextImpl(context), obj)),
            v8::Local<v8::Value>(ValueImpl::New(ToContextImpl(context), proto))
        };
        v8::TryCatch try_catch(isolate);
        setPrototype->Call(context, Null(isolate), 2, args).IsEmpty();
    }
    template<typename T>
    static inline void * PersistentData(v8::Isolate *isolate, v8::Local<T> d)
    {
        // This is a bit hacky, but should work.  We only need to store a persistent handle
        // to the function template.  We can't allocate it on the stack because it will go away
        // after this function returns.  We can't allocate it on the C++ heap because we want all
        // heap values on the V82JSC heap.  So, we will initially allocate it on the C++ heap,
        // copy the contents of the handle (which is just one pointer) to our data parameter
        // and then delete the C++ handle _without calling the destructor_.  We will reset the
        // handle, then, in the finalize phase.
        Copyable(T) * persistent = new Copyable(T)(isolate, d);
        void * data = * reinterpret_cast<void**>(persistent);
        operator delete(persistent);
        return data;
    }
    template<typename T>
    static inline void ReleasePersistentData(void *data)
    {
        Copyable(T) * persistent = reinterpret_cast<Copyable(T) *>(&data);
        persistent->Reset();
    }
    template<typename T>
    static inline v8::Local<T> FromPersistentData(v8::Isolate *isolate, void *data)
    {
        Copyable(T) * persistent = reinterpret_cast<Copyable(T) *>(&data);
        return persistent->Get(isolate);
    }
};
#define IS(name_,code_) V82JSC::is__(this,#name_,code_)

struct LocalException {
    LocalException(IsolateImpl *i) : exception_(0), isolate_(i) {}
    ~LocalException()
    {
        if (isolate_->m_handlers && exception_) {
            reinterpret_cast<TryCatchCopy*>(isolate_->m_handlers)->exception_ = (void*)exception_;
            isolate_->ii.thread_local_top()->scheduled_exception_ =
                isolate_->ii.heap()->root(v8::internal::Heap::RootListIndex::kTheHoleValueRootIndex);
        }
    }
    inline JSValueRef* operator&()
    {
        return &exception_;
    }
    inline bool ShouldThow() { return exception_ != nullptr; }
    inline void Clear() { exception_ = nullptr; }
    JSValueRef exception_;
    IsolateImpl *isolate_;
};

void proxyArrayBuffer(ContextImpl *ctx);
bool InstallAutoExtensions(v8::Local<v8::Context> context, std::map<std::string, bool>& loaded_extensions);
bool InstallExtension(v8::Local<v8::Context> context, const char *extension_name, std::map<std::string, bool>& loaded_extensions);
TrackedObjectImpl* makePrivateInstance(IsolateImpl* iso, JSContextRef ctx, JSObjectRef object);
TrackedObjectImpl* getPrivateInstance(JSContextRef ctx, JSObjectRef object);
TrackedObjectImpl* makePrivateInstance(IsolateImpl* iso, JSContextRef ctx);
void setPrivateInstance(IsolateImpl* iso, JSContextRef ctx, TrackedObjectImpl* impl, JSObjectRef object);

#endif /* V82JSC_h */
