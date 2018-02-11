//
//  ArrayBuffer.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include <v8.h>

using namespace v8;

ArrayBuffer::Allocator * ArrayBuffer::Allocator::NewDefaultAllocator()
{
    return nullptr;
}

size_t ArrayBuffer::ByteLength() const
{
    return 0;
}

/**
 * Create a new ArrayBuffer. Allocate |byte_length| bytes.
 * Allocated memory will be owned by a created ArrayBuffer and
 * will be deallocated when it is garbage-collected,
 * unless the object is externalized.
 */
Local<ArrayBuffer> ArrayBuffer::New(Isolate* isolate, size_t byte_length)
{
    return Local<ArrayBuffer>();
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
    return Local<ArrayBuffer>();
}

/**
 * Returns true if ArrayBuffer is externalized, that is, does not
 * own its memory block.
 */
bool ArrayBuffer::IsExternal() const
{
    return false;
}

/**
 * Returns true if this ArrayBuffer may be neutered.
 */
bool ArrayBuffer::IsNeuterable() const
{
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
    ArrayBuffer::Contents foo;
    return foo;
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
    ArrayBuffer::Contents foo;
    return foo;
}

/**
 * Data length in bytes.
 */
size_t SharedArrayBuffer::ByteLength() const
{
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
    return Local<SharedArrayBuffer>();
}

/**
 * Returns true if SharedArrayBuffer is externalized, that is, does not
 * own its memory block.
 */
bool SharedArrayBuffer::IsExternal() const
{
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
    return SharedArrayBuffer::Contents();
}

/**
 * Returns underlying ArrayBuffer.
 */
Local<ArrayBuffer> ArrayBufferView::Buffer()
{
    return Local<ArrayBuffer>();
}
/**
 * Byte offset in |Buffer|.
 */
size_t ArrayBufferView::ByteOffset()
{
    return 0;
}
/**
 * Size of a view in bytes.
 */
size_t ArrayBufferView::ByteLength()
{
    return 0;
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
    return 0;
}

/**
 * Returns true if ArrayBufferView's backing ArrayBuffer has already been
 * allocated.
 */
bool ArrayBufferView::HasBuffer() const
{
    return false;
}

Local<DataView> DataView::New(Local<ArrayBuffer> array_buffer,
                           size_t byte_offset, size_t length)
{
    return Local<DataView>();
}
Local<DataView> DataView::New(Local<SharedArrayBuffer> shared_array_buffer,
                           size_t byte_offset, size_t length)
{
    return Local<DataView>();
}

