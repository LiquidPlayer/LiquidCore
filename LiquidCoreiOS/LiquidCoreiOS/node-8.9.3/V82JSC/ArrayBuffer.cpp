//
//  ArrayBuffer.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;
#define H V82JSC_HeapObject

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
 * Reserved |length| bytes, but do not commit the memory. Must call
 * |SetProtection| to make memory accessible.
 */
// TODO(eholk): make this pure virtual once blink implements this.
void* ArrayBuffer::Allocator::Reserve(size_t length)
{
    assert(0);
    return nullptr;
}

/**
 * Free the memory block of size |length|, pointed to by |data|.
 * That memory is guaranteed to be previously allocated by |Allocate| or
 * |Reserve|, depending on |mode|.
 */
// TODO(eholk): make this pure virtual once blink implements this.
void ArrayBuffer::Allocator::Free(void* data, size_t length, AllocationMode mode)
{
    assert(0);
    Free(data, length);
}

/**
 * Change the protection on a region of memory.
 *
 * On platforms that make a distinction between reserving and committing
 * memory, changing the protection to kReadWrite must also ensure the memory
 * is committed.
 */
// TODO(eholk): make this pure virtual once blink implements this.
void ArrayBuffer::Allocator::SetProtection(void* data, size_t length,
                           Protection protection)
{
    assert(0);
}


ArrayBuffer::Allocator * ArrayBuffer::Allocator::NewDefaultAllocator()
{
    return new DefaultAllocator();
}

size_t ArrayBuffer::ByteLength() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    
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
void proxyArrayBuffer(GlobalContextImpl *ctx)
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
        JSObjectRef f = JSObjectMake(ctx->m_ctxRef, claz, (void*)V82JSC::ToIsolateImpl(ctx));
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
            return V82JSC::ToJSValueRef(array_buffer, isolate);
        }
        return NULL;
    });
    JSValueRef args[] = {
        JSContextGetGlobalObject(ctx->m_ctxRef),
        handler
    };
    V82JSC::exec(ctx->m_ctxRef, "_1.ArrayBuffer = new Proxy(ArrayBuffer, _2)", 2, args);
}

/**
 * Create a new ArrayBuffer. Allocate |byte_length| bytes.
 * Allocated memory will be owned by a created ArrayBuffer and
 * will be deallocated when it is garbage-collected,
 * unless the object is externalized.
 */
Local<ArrayBuffer> ArrayBuffer::New(Isolate* isolate, size_t byte_length)
{
    IsolateImpl* isolateimpl = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    LocalException exception(isolateimpl);
    
    TrackedObjectImpl *wrap = makePrivateInstance(isolateimpl, ctx);
    Local<TrackedObject> local = V82JSC::CreateLocal<TrackedObject>(&isolateimpl->ii, wrap);

    wrap->ArrayBufferInfo.buffer = isolateimpl->m_params.array_buffer_allocator->Allocate(byte_length);
    wrap->ArrayBufferInfo.byte_length = byte_length;
    wrap->ArrayBufferInfo.isExternal = false;
    wrap->ArrayBufferInfo.iso = isolateimpl;
    wrap->ArrayBufferInfo.m_self.Reset(V82JSC::ToIsolate(isolateimpl), local);
    
    JSObjectRef array_buffer = JSObjectMakeArrayBufferWithBytesNoCopy(ctx,  wrap->ArrayBufferInfo.buffer,
                                                                      byte_length, [](void* bytes, void* deallocatorContext)
    {
        TrackedObjectImpl *impl = reinterpret_cast<TrackedObjectImpl*>(deallocatorContext);
        if (!impl->ArrayBufferInfo.isExternal) {
            impl->ArrayBufferInfo.iso->m_params.array_buffer_allocator->Free(impl->ArrayBufferInfo.buffer,
                                                                             impl->ArrayBufferInfo.byte_length);
            impl->ArrayBufferInfo.buffer = nullptr;
        }
        impl->ArrayBufferInfo.m_self.Reset();
    }, (void*) wrap, &exception);
    
    setPrivateInstance(isolateimpl, ctx, wrap, array_buffer);

    JSValueRef excp = 0;
    wrap->m_internal_fields_array = JSObjectMakeArray(ctx, 0, nullptr, &excp);
    assert(excp==0);
    wrap->m_num_internal_fields = ArrayBuffer::kInternalFieldCount;
    JSValueRef zero = V82JSC::ToJSValueRef(External::New(isolate, nullptr), context);
    for (int i=0; i<ArrayBuffer::kInternalFieldCount; i++) {
        JSObjectSetPropertyAtIndex(ctx, wrap->m_internal_fields_array, i, zero, 0);
    }
    
    Local<ArrayBuffer> buffer = ValueImpl::New(V82JSC::ToContextImpl(context),
                                               array_buffer,
                                               isolateimpl->m_array_buffer_map).As<ArrayBuffer>();
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(buffer);
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
    IsolateImpl* isolateimpl = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    LocalException exception(isolateimpl);
    
    TrackedObjectImpl *wrap = makePrivateInstance(isolateimpl, ctx);
    Local<v8::TrackedObject> local = V82JSC::CreateLocal<v8::TrackedObject>(&isolateimpl->ii, wrap);

    wrap->ArrayBufferInfo.buffer = data;
    wrap->ArrayBufferInfo.byte_length = byte_length;
    wrap->ArrayBufferInfo.isExternal = mode==ArrayBufferCreationMode::kExternalized;
    wrap->ArrayBufferInfo.iso = isolateimpl;
    wrap->ArrayBufferInfo.m_self.Reset(V82JSC::ToIsolate(isolateimpl), local);

    JSObjectRef array_buffer = JSObjectMakeArrayBufferWithBytesNoCopy(ctx,  wrap->ArrayBufferInfo.buffer,
                                                                      byte_length, [](void* bytes, void* deallocatorContext)
    {
        TrackedObjectImpl *impl = reinterpret_cast<TrackedObjectImpl*>(deallocatorContext);
        if (!impl->ArrayBufferInfo.isExternal) {
            impl->ArrayBufferInfo.iso->m_params.array_buffer_allocator->Free(impl->ArrayBufferInfo.buffer,
                                                                           impl->ArrayBufferInfo.byte_length);
            impl->ArrayBufferInfo.buffer = nullptr;
        }
    }, (void*) wrap, &exception);
    
    setPrivateInstance(isolateimpl, ctx, wrap, array_buffer);
    
    JSValueRef excp = 0;
    wrap->m_internal_fields_array = JSObjectMakeArray(ctx, 0, nullptr, &excp);
    assert(excp==0);
    wrap->m_num_internal_fields = ArrayBuffer::kInternalFieldCount;
    JSValueRef zero = V82JSC::ToJSValueRef(External::New(isolate, nullptr), context);
    for (int i=0; i<ArrayBuffer::kInternalFieldCount; i++) {
        JSObjectSetPropertyAtIndex(ctx, wrap->m_internal_fields_array, i, zero, 0);
    }

    Local<ArrayBuffer> buffer = ValueImpl::New(V82JSC::ToContextImpl(context),
                                               array_buffer,
                                               isolateimpl->m_array_buffer_map).As<ArrayBuffer>();
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(buffer);
    i::Handle<i::JSArrayBuffer> buf = v8::Utils::OpenHandle(reinterpret_cast<ArrayBuffer*>(impl));
    buf->set_is_neuterable(false);
    return buffer;
}

TrackedObjectImpl * GetTrackedObject(const ArrayBuffer *ab)
{
    ValueImpl* impl = V82JSC::ToImpl<ValueImpl,ArrayBuffer>(ab);
    Isolate* isolate = V82JSC::ToIsolate(ab);
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);

    TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)impl->m_value);
    return wrap;
}

/**
 * Returns true if ArrayBuffer is externalized, that is, does not
 * own its memory block.
 */
bool ArrayBuffer::IsExternal() const
{
    return GetTrackedObject(this)->ArrayBufferInfo.isExternal;
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
    TrackedObjectImpl *wrap = GetTrackedObject(this);
    ArrayBuffer::Contents contents;
    contents.data_ = wrap->ArrayBufferInfo.buffer;
    contents.byte_length_ = wrap->ArrayBufferInfo.byte_length;
    wrap->ArrayBufferInfo.isExternal = true;
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
    TrackedObjectImpl *wrap = GetTrackedObject(this);
    ArrayBuffer::Contents contents;
    contents.data_ = wrap->ArrayBufferInfo.buffer;
    contents.byte_length_ = wrap->ArrayBufferInfo.byte_length;
    return contents;
}

/**
 * Data length in bytes.
 */
size_t SharedArrayBuffer::ByteLength() const
{
    // SharedArrayBuffer is implemented but not compatible
    assert(0);
    return 0;
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
    assert(0);
    return Local<SharedArrayBuffer>();
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
    assert(0);
    return Local<SharedArrayBuffer>();
}

/**
 * Returns true if SharedArrayBuffer is externalized, that is, does not
 * own its memory block.
 */
bool SharedArrayBuffer::IsExternal() const
{
    // Not supported in JSC
    return false;
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
    return SharedArrayBuffer::Contents();
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
    return SharedArrayBuffer::Contents();
}

/**
 * Returns underlying ArrayBuffer.
 */
Local<ArrayBuffer> ArrayBufferView::Buffer()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    
    JSStringRef sbuffer = JSStringCreateWithUTF8CString("buffer");
    JSValueRef excp = 0;
    JSValueRef buffer = JSObjectGetProperty(V82JSC::ToContextRef(context), (JSObjectRef)value, sbuffer, &excp);
    assert(excp==0);
    JSStringRelease(sbuffer);
    return ValueImpl::New(V82JSC::ToContextImpl(context), buffer).As<ArrayBuffer>();
}
/**
 * Byte offset in |Buffer|.
 */
size_t ArrayBufferView::ByteOffset()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    
    JSStringRef soffset = JSStringCreateWithUTF8CString("byteOffset");
    JSValueRef excp = 0;
    JSValueRef offset = JSObjectGetProperty(V82JSC::ToContextRef(context), (JSObjectRef)value, soffset, &excp);
    assert(excp==0);
    JSStringRelease(soffset);
    size_t byte_offset = JSValueToNumber(V82JSC::ToContextRef(context), offset, &excp);
    assert(excp==0);
    return byte_offset;
}
/**
 * Size of a view in bytes.
 */
size_t ArrayBufferView::ByteLength()
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);

    JSStringRef slength = JSStringCreateWithUTF8CString("byteLength");
    JSValueRef excp = 0;
    JSValueRef length = JSObjectGetProperty(V82JSC::ToContextRef(context), (JSObjectRef)value, slength, &excp);
    assert(excp==0);
    JSStringRelease(slength);
    size_t byte_length = JSValueToNumber(V82JSC::ToContextRef(context), length, &excp);
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
    assert(0);
    return 0;
}

/**
 * Returns true if ArrayBufferView's backing ArrayBuffer has already been
 * allocated.
 */
bool ArrayBufferView::HasBuffer() const
{
    assert(0);
    return false;
}

Local<DataView> DataView::New(Local<ArrayBuffer> array_buffer,
                           size_t byte_offset, size_t length)
{
    Local<Context> context = V82JSC::ToCurrentContext(*array_buffer);
    JSValueRef ab = V82JSC::ToJSValueRef(array_buffer, context);

    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef args[] = {
        ab,
        JSValueMakeNumber(ctx, byte_offset),
        JSValueMakeNumber(ctx, length)
    };
    JSObjectRef data_view = (JSObjectRef) V82JSC::exec(ctx, "return new DataView(_1,_2,_3)", 3, args);
    Local<DataView> view = ValueImpl::New(V82JSC::ToContextImpl(context), data_view).As<DataView>();
    return view;
}
Local<DataView> DataView::New(Local<SharedArrayBuffer> shared_array_buffer,
                           size_t byte_offset, size_t length)
{
    // SharedArrayBuffer is implemented but not compatible
    assert(0);
    return Local<DataView>();
}

