/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_ArrayBuffer_h
#define V82JSC_ArrayBuffer_h

namespace V82JSC {

void proxyArrayBuffer(V82JSC::GlobalContext *ctx);

struct ArrayBufferInfo{
    void *buffer;
    size_t byte_length;
    bool isExternal;
    bool isNeuterable;
    intptr_t isolate_id;
    v8::ArrayBuffer::Allocator* array_buffer_allocator;
    static std::map<intptr_t, std::map<void *, ArrayBufferInfo*>> s_array_buffer_infos;
    static intptr_t s_isolate_counter;
    static std::mutex s_array_buffer_info_mutex;

    static intptr_t NewIsolate();
    static void DisposeIsolate(intptr_t isolate_id);
    static void Deallocate(void* bytes, void* deallocatorContext);
    
    ArrayBufferInfo(intptr_t isolate_id,
                    size_t length,
                    v8::ArrayBuffer::Allocator* allocator,
                    void *data = nullptr);
};

}

#endif /* V82JSC_ArrayBuffer_h */
