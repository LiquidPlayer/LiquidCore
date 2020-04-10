/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "Object.h"
#include "ArrayBuffer.h"

using namespace V82JSC;
using v8::ArrayBuffer;
using v8::SharedArrayBuffer;
using v8::ArrayBufferView;
using v8::DataView;
using v8::Isolate;
using v8::Local;

class DefaultAllocator : public ArrayBuffer::Allocator
{
    /**
     * Allocate |length| bytes. Return NULL if allocation is not successful.
     * Memory should be initialized to zeroes.
     */
    virtual void* Allocate(size_t length)
    {
        void *mem = malloc(length);
        memset(mem, 0, length);
        return mem;
    }
    
    /**
     * Allocate |length| bytes. Return NULL if allocation is not successful.
     * Memory does not have to be initialized.
     */
    virtual void* AllocateUninitialized(size_t length)
    {
        return malloc(length);
    }
    
    /**
     * Free the memory block of size |length|, pointed to by |data|.
     * That memory is guaranteed to be previously allocated by |Allocate|.
     */
    virtual void Free(void* data, size_t length)
    {
        free(data);
    }
};

/**
 * Free the memory block of size |length|, pointed to by |data|.
 * That memory is guaranteed to be previously allocated by |Allocate| or
 * |Reserve|, depending on |mode|.
 */
// TODO(eholk): make this pure virtual once blink implements this.
void ArrayBuffer::Allocator::Free(void* data, size_t length)
{
    assert(0);
    Free(data, length);
}

ArrayBuffer::Allocator * ArrayBuffer::Allocator::NewDefaultAllocator()
{
    return new DefaultAllocator();
}

size_t ArrayBuffer::ByteLength() const
{
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    JSValueRef value = ToJSValueRef(this, context);
    
    JSValueRef excp = 0;
    size_t length = JSObjectGetArrayBufferByteLength(ctx, (JSObjectRef)value, &excp);
    assert(excp==0);
    return length;
}

#define CALLBACK_PARAMS JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, \
size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception

/*
 * In order to control the lifecycle of the ArrayBuffer's backing store, we need to do the creation
 * from the API, even if called from JS.  So we proxy the global ArrayBuffer object and capture the
 * constructor.
 */
void V82JSC::proxyArrayBuffer(GlobalContext *ctx)
{
    JSObjectRef handler = JSObjectMake(ctx->m_ctxRef, nullptr, nullptr);
    auto handler_func = [ctx, handler](const char *name, JSObjectCallAsFunctionCallback callback) -> void {
        JSValueRef excp = 0;
        JSStringRef sname = JSStringCreateWithUTF8CString(name);
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        def.callAsFunction = callback;
        def.className = name;
        JSClassRef claz = JSClassCreate(&def);
        JSObjectRef f = JSObjectMake(ctx->m_ctxRef, claz, (void*)ToIsolateImpl(ctx));
        JSObjectSetProperty(ctx->m_ctxRef, handler, sname, f, 0, &excp);
        JSStringRelease(sname);
        assert(excp==0);
    };
    
    handler_func("construct", [](CALLBACK_PARAMS) -> JSValueRef
    {
        Isolate* isolate = (Isolate*) JSObjectGetPrivate(function);
        assert(argumentCount>1);
        size_t byte_length = 0;
        if (JSValueIsArray(ctx, arguments[1])) {
            JSValueRef excp=0;
            JSValueRef length = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)arguments[1], 0, &excp);
            assert(excp==0);
            if (JSValueIsNumber(ctx, length)) {
                byte_length = JSValueToNumber(ctx, length, &excp);
                assert(excp==0);
            }
        }
        if (!exception || !*exception) {
            Local<ArrayBuffer> array_buffer = ArrayBuffer::New(isolate, byte_length);
            return ToJSValueRef(array_buffer, isolate);
        }
        return NULL;
    });
    JSValueRef args[] = {
        JSContextGetGlobalObject(ctx->m_ctxRef),
        handler
    };
    exec(ctx->m_ctxRef, "_1.ArrayBuffer = new Proxy(ArrayBuffer, _2)", 2, args);
}

ArrayBufferInfo::ArrayBufferInfo(intptr_t id, size_t length,
                                 v8::ArrayBuffer::Allocator* allocator,
                                 void* data)
{
    isolate_id = id;
    byte_length = length;
    array_buffer_allocator = allocator;
    buffer = data ? data : array_buffer_allocator->Allocate(byte_length);
    std::unique_lock<std::mutex> lock(s_array_buffer_info_mutex);
    s_array_buffer_infos[id][buffer] = this;
}
intptr_t ArrayBufferInfo::NewIsolate()
{
    std::unique_lock<std::mutex> lock(s_array_buffer_info_mutex);
    auto isolate_id = s_isolate_counter ++;
    s_array_buffer_infos[isolate_id] = std::map<void*, ArrayBufferInfo*>();
    return isolate_id;
}
void ArrayBufferInfo::Deallocate(void *bytes, void* deallocatorContext)
{
    int isolate_id = reinterpret_cast<intptr_t>(deallocatorContext);
    std::unique_lock<std::mutex> lock(s_array_buffer_info_mutex);
    if (s_array_buffer_infos.count(isolate_id)) {
        auto info = s_array_buffer_infos[isolate_id][bytes];
        if (info) {
            if (!info->isExternal) {
                info->array_buffer_allocator->Free(info->buffer, info->byte_length);
            }
            s_array_buffer_infos[isolate_id].erase(bytes);
            delete info;
        }
    }
}
void ArrayBufferInfo::DisposeIsolate(intptr_t isolate_id)
{
    s_array_buffer_info_mutex.lock();
    auto& map = s_array_buffer_infos[isolate_id];
    while (!map.empty()) {
        auto bytes = (*map.begin()).first;
        s_array_buffer_info_mutex.unlock();
        Deallocate(bytes, reinterpret_cast<void*>(isolate_id));
        s_array_buffer_info_mutex.lock();
    }
    s_array_buffer_infos.erase(isolate_id);
    s_array_buffer_info_mutex.unlock();
}
std::map<intptr_t, std::map<void *, ArrayBufferInfo*>> ArrayBufferInfo::s_array_buffer_infos;
intptr_t ArrayBufferInfo::s_isolate_counter = 0;
std::mutex ArrayBufferInfo::s_array_buffer_info_mutex;

/**
 * Create a new ArrayBuffer. Allocate |byte_length| bytes.
 * Allocated memory will be owned by a created ArrayBuffer and
 * will be deallocated when it is garbage-collected,
 * unless the object is externalized.
 */
Local<ArrayBuffer> ArrayBuffer::New(Isolate* isolate, size_t byte_length)
{
    IsolateImpl* isolateimpl = ToIsolateImpl(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    
    LocalException exception(isolateimpl);
    
    auto wrap = V82JSC::TrackedObject::makePrivateInstance(isolateimpl, ctx);
    Local<TrackedObject> local = CreateLocal<TrackedObject>(&isolateimpl->ii, wrap);

    wrap->m_array_buffer_info = new V82JSC::ArrayBufferInfo(isolateimpl->m_isolate_id,
                                                            byte_length,
                                                            isolateimpl->m_params.array_buffer_allocator);
    wrap->m_array_buffer_info->isExternal = false;

    JSObjectRef array_buffer = JSObjectMakeArrayBufferWithBytesNoCopy(ctx,  wrap->m_array_buffer_info->buffer,
                                                                      byte_length, ArrayBufferInfo::Deallocate,
                                                                      (void*) wrap->m_array_buffer_info, &exception);
    
    V82JSC::TrackedObject::setPrivateInstance(isolateimpl, ctx, wrap, array_buffer);

    JSValueRef excp = 0;
    wrap->m_internal_fields_array = JSObjectMakeArray(ctx, 0, nullptr, &excp);
    assert(excp==0);
    JSValueProtect(ctx, wrap->m_internal_fields_array);
    wrap->m_num_internal_fields = ArrayBuffer::kInternalFieldCount;
    JSValueRef zero = ToJSValueRef(External::New(isolate, nullptr), context);
    for (int i=0; i<ArrayBuffer::kInternalFieldCount; i++) {
        JSObjectSetPropertyAtIndex(ctx, wrap->m_internal_fields_array, i, zero, 0);
    }
    
    Local<ArrayBuffer> buffer = V82JSC::Value::New(ToContextImpl(context),
                                               array_buffer,
                                               isolateimpl->m_array_buffer_map).As<ArrayBuffer>();
    auto impl = ToImpl<Value>(buffer);
    i::Handle<i::JSArrayBuffer> buf = v8::Utils::OpenHandle(reinterpret_cast<ArrayBuffer*>(impl));
    buf->set_is_neuterable(false);
    return buffer;
}

/**
 * Create a new ArrayBuffer over an existing memory block.
 * The created array buffer is by default immediately in externalized state.
 * In externalized state, the memory block will not be reclaimed when a
 * created ArrayBuffer is garbage-collected.
 * In internalized state, the memory block will be released using
 * |Allocator::Free| once all ArrayBuffers referencing it are collected by
 * the garbage collector.
 */
Local<ArrayBuffer> ArrayBuffer::New(
                              Isolate* isolate, void* data, size_t byte_length,
                              ArrayBufferCreationMode mode)
{
    IsolateImpl* isolateimpl = ToIsolateImpl(isolate);
    Local<Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    
    LocalException exception(isolateimpl);
    
    auto wrap = V82JSC::TrackedObject::makePrivateInstance(isolateimpl, ctx);
    Local<v8::TrackedObject> local = CreateLocal<v8::TrackedObject>(&isolateimpl->ii, wrap);

    wrap->m_array_buffer_info = new V82JSC::ArrayBufferInfo(isolateimpl->m_isolate_id,
                                                            byte_length,
                                                            isolateimpl->m_params.array_buffer_allocator,
                                                            data);
    wrap->m_array_buffer_info->isExternal = mode==ArrayBufferCreationMode::kExternalized;

    JSObjectRef array_buffer = JSObjectMakeArrayBufferWithBytesNoCopy(ctx,  wrap->m_array_buffer_info->buffer,
                                                                      byte_length, ArrayBufferInfo::Deallocate,
                                                                      (void*) wrap->m_array_buffer_info, &exception);
    
    V82JSC::TrackedObject::setPrivateInstance(isolateimpl, ctx, wrap, array_buffer);
    
    JSValueRef excp = 0;
    wrap->m_internal_fields_array = JSObjectMakeArray(ctx, 0, nullptr, &excp);
    assert(excp==0);
    JSValueProtect(ctx, wrap->m_internal_fields_array);
    wrap->m_num_internal_fields = ArrayBuffer::kInternalFieldCount;
    JSValueRef zero = ToJSValueRef(External::New(isolate, nullptr), context);
    for (int i=0; i<ArrayBuffer::kInternalFieldCount; i++) {
        JSObjectSetPropertyAtIndex(ctx, wrap->m_internal_fields_array, i, zero, 0);
    }

    Local<ArrayBuffer> buffer = V82JSC::Value::New(ToContextImpl(context),
                                               array_buffer,
                                               isolateimpl->m_array_buffer_map).As<ArrayBuffer>();
    auto impl = ToImpl<Value>(buffer);
    i::Handle<i::JSArrayBuffer> buf = v8::Utils::OpenHandle(reinterpret_cast<ArrayBuffer*>(impl));
    buf->set_is_neuterable(false);
    return buffer;
}

TrackedObject * GetTrackedObject(const ArrayBuffer *ab)
{
    auto impl = ToImpl<Value,ArrayBuffer>(ab);
    auto isolate = ToIsolate(ab);
    Local<v8::Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);

    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)impl->m_value);
    return wrap;
}

/**
 * Returns true if ArrayBuffer is externalized, that is, does not
 * own its memory block.
 */
bool ArrayBuffer::IsExternal() const
{
    return GetTrackedObject(this)->m_array_buffer_info->isExternal;
}

/**
 * Returns true if this ArrayBuffer may be neutered.
 */
bool ArrayBuffer::IsNeuterable() const
{
    // Neutering is not supported in JavaScriptCore
    return false;
}

/**
 * Neuters this ArrayBuffer and all its views (typed arrays).
 * Neutering sets the byte length of the buffer and all typed arrays to zero,
 * preventing JavaScript from ever accessing underlying backing store.
 * ArrayBuffer should have been externalized and must be neuterable.
 */
void ArrayBuffer::Neuter()
{
    // Neutering is not supported in JavaScriptCore
    // Silently fail
}

/**
 * Make this ArrayBuffer external. The pointer to underlying memory block
 * and byte length are returned as |Contents| structure. After ArrayBuffer
 * had been externalized, it does no longer own the memory block. The caller
 * should take steps to free memory when it is no longer needed.
 *
 * The memory block is guaranteed to be allocated with |Allocator::Allocate|
 * that has been set via Isolate::CreateParams.
 */
ArrayBuffer::Contents ArrayBuffer::Externalize()
{
    auto wrap = GetTrackedObject(this);
    ArrayBuffer::Contents contents;
    contents.data_ = wrap->m_array_buffer_info->buffer;
    contents.byte_length_ = wrap->m_array_buffer_info->byte_length;
    wrap->m_array_buffer_info->isExternal = true;
    return contents;
}

/**
 * Get a pointer to the ArrayBuffer's underlying memory block without
 * externalizing it. If the ArrayBuffer is not externalized, this pointer
 * will become invalid as soon as the ArrayBuffer gets garbage collected.
 *
 * The embedder should make sure to hold a strong reference to the
 * ArrayBuffer while accessing this pointer.
 *
 * The memory block is guaranteed to be allocated with |Allocator::Allocate|.
 */
ArrayBuffer::Contents ArrayBuffer::GetContents()
{
    auto wrap = GetTrackedObject(this);
    ArrayBuffer::Contents contents;
    contents.data_ = wrap->m_array_buffer_info->buffer;
    contents.byte_length_ = wrap->m_array_buffer_info->byte_length;
    return contents;
}

/**
 * Data length in bytes.
 */
size_t SharedArrayBuffer::ByteLength() const
{
    // SharedArrayBuffer is implemented but not compatible
    NOT_IMPLEMENTED;
}

/**
 * Create a new SharedArrayBuffer. Allocate |byte_length| bytes.
 * Allocated memory will be owned by a created SharedArrayBuffer and
 * will be deallocated when it is garbage-collected,
 * unless the object is externalized.
 */
Local<SharedArrayBuffer> SharedArrayBuffer::New(Isolate* isolate, size_t byte_length)
{
    // SharedArrayBuffer is implemented but not compatible
    NOT_IMPLEMENTED;
}

/**
 * Create a new SharedArrayBuffer over an existing memory block.  The created
 * array buffer is immediately in externalized state unless otherwise
 * specified. The memory block will not be reclaimed when a created
 * SharedArrayBuffer is garbage-collected.
 */
Local<SharedArrayBuffer> SharedArrayBuffer::New(
                                    Isolate* isolate, void* data, size_t byte_length,
                                    ArrayBufferCreationMode mode)
{
    // Not supported in JSC
    NOT_IMPLEMENTED;
}

/**
 * Returns true if SharedArrayBuffer is externalized, that is, does not
 * own its memory block.
 */
bool SharedArrayBuffer::IsExternal() const
{
    // Not supported in JSC
    NOT_IMPLEMENTED;
}

/**
 * Make this SharedArrayBuffer external. The pointer to underlying memory
 * block and byte length are returned as |Contents| structure. After
 * SharedArrayBuffer had been externalized, it does no longer own the memory
 * block. The caller should take steps to free memory when it is no longer
 * needed.
 *
 * The memory block is guaranteed to be allocated with |Allocator::Allocate|
 * by the allocator specified in
 * v8::Isolate::CreateParams::array_buffer_allocator.
 *
 */
SharedArrayBuffer::Contents SharedArrayBuffer::Externalize()
{
    // Not supported in JSC
    NOT_IMPLEMENTED;
}

/**
 * Get a pointer to the ArrayBuffer's underlying memory block without
 * externalizing it. If the ArrayBuffer is not externalized, this pointer
 * will become invalid as soon as the ArrayBuffer became garbage collected.
 *
 * The embedder should make sure to hold a strong reference to the
 * ArrayBuffer while accessing this pointer.
 *
 * The memory block is guaranteed to be allocated with |Allocator::Allocate|
 * by the allocator specified in
 * v8::Isolate::CreateParams::array_buffer_allocator.
 */
SharedArrayBuffer::Contents SharedArrayBuffer::GetContents()
{
    // Not supported in JSC
    NOT_IMPLEMENTED;
}

/**
 * Returns underlying ArrayBuffer.
 */
Local<ArrayBuffer> ArrayBufferView::Buffer()
{
    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    
    JSStringRef sbuffer = JSStringCreateWithUTF8CString("buffer");
    JSValueRef excp = 0;
    JSValueRef buffer = JSObjectGetProperty(ToContextRef(context), (JSObjectRef)value, sbuffer, &excp);
    assert(excp==0);
    JSStringRelease(sbuffer);
    return V82JSC::Value::New(ToContextImpl(context), buffer).As<ArrayBuffer>();
}
/**
 * Byte offset in |Buffer|.
 */
size_t ArrayBufferView::ByteOffset()
{
    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    
    JSStringRef soffset = JSStringCreateWithUTF8CString("byteOffset");
    JSValueRef excp = 0;
    JSValueRef offset = JSObjectGetProperty(ToContextRef(context), (JSObjectRef)value, soffset, &excp);
    assert(excp==0);
    JSStringRelease(soffset);
    size_t byte_offset = JSValueToNumber(ToContextRef(context), offset, &excp);
    assert(excp==0);
    return byte_offset;
}
/**
 * Size of a view in bytes.
 */
size_t ArrayBufferView::ByteLength()
{
    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);

    JSStringRef slength = JSStringCreateWithUTF8CString("byteLength");
    JSValueRef excp = 0;
    JSValueRef length = JSObjectGetProperty(ToContextRef(context), (JSObjectRef)value, slength, &excp);
    assert(excp==0);
    JSStringRelease(slength);
    size_t byte_length = JSValueToNumber(ToContextRef(context), length, &excp);
    assert(excp==0);
    return byte_length;
}

/**
 * Copy the contents of the ArrayBufferView's buffer to an embedder defined
 * memory without additional overhead that calling ArrayBufferView::Buffer
 * might incur.
 *
 * Will write at most min(|byte_length|, ByteLength) bytes starting at
 * ByteOffset of the underlying buffer to the memory starting at |dest|.
 * Returns the number of bytes actually written.
 */
size_t ArrayBufferView::CopyContents(void* dest, size_t byte_length)
{
    NOT_IMPLEMENTED;
}

/**
 * Returns true if ArrayBufferView's backing ArrayBuffer has already been
 * allocated.
 */
bool ArrayBufferView::HasBuffer() const
{
    NOT_IMPLEMENTED;
}

Local<DataView> DataView::New(Local<ArrayBuffer> array_buffer,
                           size_t byte_offset, size_t length)
{
    Local<Context> context = ToCurrentContext(*array_buffer);
    JSValueRef ab = ToJSValueRef(array_buffer, context);

    JSContextRef ctx = ToContextRef(context);
    JSValueRef args[] = {
        ab,
        JSValueMakeNumber(ctx, byte_offset),
        JSValueMakeNumber(ctx, length)
    };
    JSObjectRef data_view = (JSObjectRef) exec(ctx, "return new DataView(_1,_2,_3)", 3, args);
    Local<DataView> view = V82JSC::Value::New(ToContextImpl(context), data_view).As<DataView>();
    return view;
}
Local<DataView> DataView::New(Local<SharedArrayBuffer> shared_array_buffer,
                           size_t byte_offset, size_t length)
{
    // SharedArrayBuffer is implemented but not compatible
    NOT_IMPLEMENTED;
}

