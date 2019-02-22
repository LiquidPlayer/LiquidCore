/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_h
#define V82JSC_h

#include "Isolate.h"
#include "Context.h"
#include "Value.h"
#include "JSContextRefPrivate.h"
#include "JSObjectRefPrivate.h"

using v8::internal::IsolateImpl;

namespace V82JSC {

template <class T>
class _maybe {
public:
    bool has_value_;
    T value_;
    inline v8::Maybe<T> toMaybe() { return *(reinterpret_cast<v8::Maybe<T> *>(this)); }
    explicit _maybe(const T& t) : has_value_(true), value_(t) {}
    _maybe() : has_value_(false) {}
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

struct LocalException {
    LocalException(IsolateImpl *i) : exception_(0), isolate_(i) {}
    ~LocalException();
    inline JSValueRef* operator&()
    {
        return &exception_;
    }
    inline bool ShouldThow() { return exception_ != nullptr; }
    inline void Clear() { exception_ = nullptr; }
    JSValueRef exception_;
    IsolateImpl *isolate_;
};

template <class T>
class __local {
public:
    T* val_;
    v8::Local<T> toLocal() { return *(reinterpret_cast<v8::Local<T> *>(this)); }
    __local(void *v) { val_ = reinterpret_cast<T*>(v); }
};

inline JSContextRef ToContextRef(v8::Local<v8::Context> context);
inline IsolateImpl* ToIsolateImpl(const V82JSC::HeapObject *io);
inline v8::Local<v8::Context> OperatingContext(v8::Isolate* isolate);

template <class T>
inline v8::Local<T> CreateLocal(v8::internal::Isolate *isolate, V82JSC::HeapObject *o)
{
    v8::internal::Object ** handle =
        v8::internal::HandleScope::CreateHandle(isolate,
            reinterpret_cast<v8::internal::Object*>(reinterpret_cast<intptr_t>(o) + v8::internal::kHeapObjectTag));
    return __local<T>(handle).toLocal();
}
template <class T>
inline v8::Local<T> CreateLocal(v8::Isolate *isolate, V82JSC::HeapObject *o)
{
    return CreateLocal<T>(reinterpret_cast<v8::internal::Isolate*>(isolate), o);
}
template <class T>
inline v8::Local<T> CreateLocalSmi(v8::internal::Isolate *isolate, v8::internal::Smi* smi)
{
    v8::internal::Object ** handle = v8::internal::HandleScope::CreateHandle(isolate, smi);
    return __local<T>(handle).toLocal();
}

template<class T>
inline JSValueRef ToJSValueRef_(v8::internal::Object *obj, v8::Local<v8::Context> context)
{
    JSContextRef ctx = ToContextRef(context);
    if (obj->IsSmi()) {
        int value = v8::internal::Smi::ToInt(obj);
        return JSValueMakeNumber(ctx, value);
    } else {
        auto *that_ = reinterpret_cast<V82JSC::HeapObject*>
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
        return reinterpret_cast<Value*>(that_)->m_value;
    }
}
template <class T>
inline JSValueRef ToJSValueRef(v8::Local<T> v, v8::Local<v8::Context> context)
{
    if (v.IsEmpty()) return 0;
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(*v);
    return ToJSValueRef_<T>(obj, context);
}
template<class T>
inline JSValueRef ToJSValueRef(const T* v, v8::Local<v8::Context> context)
{
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(v));
    return ToJSValueRef_<T>(obj, context);
}
template <class T>
inline JSValueRef ToJSValueRef(v8::Local<T> v, v8::Isolate *isolate)
{
    return ToJSValueRef(v, OperatingContext(isolate));
}
template <class T>
inline JSValueRef ToJSValueRef(const T* v, v8::Isolate *isolate)
{
    return ToJSValueRef(v, OperatingContext(isolate));
}

template <class I, class T>
inline I* ToImpl(v8::Local<T> v)
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
inline I* ToImpl(const T* thiz)
{
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(thiz));
    if (!obj->IsSmi()) {
        I *that_ = reinterpret_cast<I*>(reinterpret_cast<intptr_t>(obj) & ~3);
        return that_;
    }
    return nullptr;
}

inline V82JSC::Context* ToContextImpl(v8::Local<v8::Context> context)
{
    return ToImpl<V82JSC::Context, v8::Context>(context);
}
inline V82JSC::Context* ToContextImpl(const v8::Context* thiz)
{
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<v8::Context*>(thiz));
    assert(!obj->IsSmi());
    V82JSC::Context *ctx = reinterpret_cast<V82JSC::Context*>(reinterpret_cast<intptr_t>(obj) & ~3);
    return ctx;
}

inline GlobalContext* ToGlobalContextImpl(v8::Local<v8::Context> context)
{
    return ToImpl<GlobalContext, v8::Context>(context);
}
inline GlobalContext* ToGlobalContextImpl(const v8::Context* thiz)
{
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<v8::Context*>(thiz));
    assert(!obj->IsSmi());
    GlobalContext *ctx = reinterpret_cast<GlobalContext*>(reinterpret_cast<intptr_t>(obj) & ~3);
    return ctx;
}

inline JSContextRef ToContextRef(v8::Local<v8::Context> context)
{
    return ToContextImpl(context)->m_ctxRef;
}
inline JSContextRef ToContextRef(v8::Isolate *isolate)
{
    return ToContextImpl(OperatingContext(isolate))->m_ctxRef;
}

inline IsolateImpl* ToIsolateImpl(v8::Isolate *isolate)
{
    return reinterpret_cast<IsolateImpl*>(isolate);
}
inline IsolateImpl* ToIsolateImpl(const V82JSC::HeapObject *io)
{
    v8::internal::Isolate* isolate =
        reinterpret_cast<v8::internal::HeapObject*>
        (V82JSC::ToHeapPointer(const_cast<V82JSC::HeapObject *>(io)))->GetIsolate();
    return reinterpret_cast<IsolateImpl*>(isolate);
}
inline v8::Isolate* ToIsolate(IsolateImpl *isolate)
{
    return reinterpret_cast<v8::Isolate*>(isolate);
}
inline IsolateImpl* ToIsolateImpl(const v8::Value *val)
{
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<v8::Value*>(val));
    assert(!obj->IsSmi());
    v8::internal::Isolate* isolate = reinterpret_cast<v8::internal::HeapObject*>(obj)->GetIsolate();
    return reinterpret_cast<IsolateImpl*>(isolate);
}

template <class T>
inline v8::Isolate* ToIsolate(const T* thiz)
{
    v8::internal::Object *obj = * reinterpret_cast<v8::internal::Object**>(const_cast<T*>(thiz));
    if (obj->IsSmi()) {
        return v8::Isolate::GetCurrent();
    } else {
        v8::internal::Isolate *i = reinterpret_cast<v8::internal::HeapObject*>(obj)->GetIsolate();
        return reinterpret_cast<v8::Isolate*>(i);
    }
}

inline v8::Local<v8::Context> OperatingContext(v8::Isolate* isolate)
{
    v8::EscapableHandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    if (context.IsEmpty()) {
        context = v8::Local<v8::Context>::New(isolate, ToIsolateImpl(isolate)->m_nullContext);
    }
    return scope.Escape(context);
}
template <class T>
inline v8::Local<v8::Context> ToCurrentContext(const T* thiz)
{
    return OperatingContext(ToIsolate<T>(thiz));
}

JSObjectRef make_exec_function(JSGlobalContextRef ctx, const char *body, int argc);

inline JSValueRef exec(JSContextRef ctx, const char *body, int argc,
                              const JSValueRef *argv, JSValueRef *pexcp=nullptr)
{
    JSGlobalContextRef gctx = JSContextGetGlobalContext(ctx);
    IsolateImpl* iso;
    {
        std::unique_lock<std::mutex> lk(IsolateImpl::s_isolate_mutex);
        iso = IsolateImpl::s_context_to_isolate_map[gctx];
    }
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

inline v8::Local<v8::Context> FindGlobalContext(v8::Local<v8::Context> context)
{
    v8::Isolate* isolate = ToIsolate(ToContextImpl(context));
    v8::EscapableHandleScope scope(isolate);
    
    IsolateImpl *i = ToIsolateImpl(isolate);
    JSGlobalContextRef gctx = JSContextGetGlobalContext(ToContextRef(context));
    if (i->m_global_contexts.count(gctx) > 0) {
        return scope.Escape(i->m_global_contexts[gctx].Get(isolate));
    }
    return v8::Local<v8::Context>();
}
JSValueRef GetRealPrototype(v8::Local<v8::Context> context, JSObjectRef obj);
void SetRealPrototype(v8::Local<v8::Context> context, JSObjectRef obj, JSValueRef proto,
                             bool override_immutable=false);
template<typename T>
inline void * PersistentData(v8::Isolate *isolate, v8::Local<T> d)
{
    // This is a bit hacky, but should work.  We only need to store a persistent handle
    // to the function template.  We can't allocate it on the stack because it will go away
    // after this function returns.  We can't allocate it on the C++ heap because we want all
    // heap values on the V82JSC heap.  So, we will initially allocate it on the C++ heap,
    // copy the contents of the handle (which is just one pointer) to our data parameter
    // and then delete the C++ handle _without calling the destructor_.  We will reset the
    // handle, then, in the finalize phase.
    v8::Persistent<T> * persistent = new v8::Persistent<T>(isolate, d);
    void * data = * reinterpret_cast<void**>(persistent);
    operator delete(persistent);
    return data;
}
template<typename T>
inline void ReleasePersistentData(void *data)
{
    v8::Persistent<T> * persistent = reinterpret_cast<v8::Persistent<T> *>(&data);
    persistent->Reset();
}
template<typename T>
inline v8::Local<T> FromPersistentData(v8::Isolate *isolate, void *data)
{
    v8::EscapableHandleScope scope(isolate);
    
    v8::Persistent<T> * persistent = reinterpret_cast<v8::Persistent<T> *>(&data);
    return scope.Escape(persistent->Get(isolate));
}
inline IsolateImpl* IsolateFromCtx(JSContextRef lctx)
{
    JSGlobalContextRef ctx = JSContextGetGlobalContext(lctx);
    std::lock_guard<std::mutex> lk(IsolateImpl::s_isolate_mutex);
    return IsolateImpl::s_context_to_isolate_map[ctx];
}

} /* namespace V82JSC */

#endif /* V82JSC_h */
