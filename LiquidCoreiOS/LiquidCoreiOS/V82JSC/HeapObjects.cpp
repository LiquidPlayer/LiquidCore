
//  HeapObjects.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "HeapObjects.h"

using namespace V82JSC_HeapObject;
using namespace v8;

HeapObject * HeapAllocator::Alloc(IsolateImpl *isolate, const BaseMap *map, uint32_t size)
{
    internal::Heap *heap = reinterpret_cast<internal::Isolate*>(isolate)->heap();
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(heap);
    HeapAllocator *chunk = static_cast<HeapAllocator*>(heapimpl->m_heap_top);
    void *alloc = nullptr;
    int index = 0;
    int pos = 0;
    
    assert(map || size > 0);
    if (size == 0) {
        size = map->size;
    }
    assert(size <= ReserveSize(size));
    uint32_t slots = (size <= (HEAP_SLOT_SIZE>>1)) ? 1 : 1 << (LogReserveSize(size) - HEAP_SLOT_SIZE_LOG);
    assert(slots > 0);
    uint32_t actual_used_slots = ((size - 1) / HEAP_SLOT_SIZE) + 1;

    while (!alloc) {
        if (chunk) {
            pos = 0;
            int log = LogReserveSize(size) - HEAP_SLOT_SIZE_LOG;
            if (log < HEAP_SMALL_SPACE_LOG) {
                Transform xform = transform(slots);
                for (index=chunk->info.m_small_indicies[log];
                     index>=0 && (pos = xform(chunk->alloc_map[index])) == 0;
                     --index);
                if (pos == 0) {
                    for (index=HEAP_BLOCKS-1;
                         index>chunk->info.m_small_indicies[log] && (pos = xform(chunk->alloc_map[index])) == 0;
                         --index);
                }
                if (pos) {
                    int slot = (index * 64) + (pos-1);
                    assert(slot < HEAP_SLOTS);
                    alloc = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(chunk) +
                                                    slot * HEAP_SLOT_SIZE);
                    assert(alloc > chunk && alloc < (void*)(reinterpret_cast<intptr_t>(chunk) + HEAP_ALIGNMENT));
                    uint64_t mask = (((uint64_t)1<<actual_used_slots)-1) << (pos-1);
                    assert((chunk->alloc_map[index] & mask) == 0);
                    chunk->alloc_map[index] |= mask;
                    chunk->info.m_small_indicies[log] = index;
                }
            } else {
                int blocks_needed = (((int)size-1) / HEAP_BLOCK_SIZE) + 1;
                int count = 0;
                for (index = chunk->info.m_large_index;
                     count < blocks_needed && index < HEAP_BLOCKS;
                     ++index) {
                    
                    if (chunk->alloc_map[index] == 0) count++;
                    else count = 0;
                }
                if (count < blocks_needed) {
                    count = 0;
                    for (index = 0;
                         count < blocks_needed && index < chunk->info.m_large_index;
                         ++index) {
                        
                        if (chunk->alloc_map[index] == 0) count++;
                        else count = 0;
                    }
                }
                if (count == blocks_needed) {
                    int slot = (index * 64);
                    alloc = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(chunk) +
                                                    (slot * HEAP_SLOT_SIZE));
                    for (int i=0; i<blocks_needed; i++) {
                        uint64_t mask = ~0;
                        if (actual_used_slots < 64) {
                            mask = (((uint64_t)1<<actual_used_slots)-1) << (pos-1);
                            actual_used_slots = 0;
                        } else {
                            actual_used_slots -= 64;
                        }
                        chunk->alloc_map[index-i-1] = mask;
                    }
                    chunk->info.m_large_index = index;
                }
            }
            if (!alloc)
                chunk = static_cast<HeapAllocator*>(chunk->next_chunk());
        }
        
        if (!alloc && !chunk) {
            void *ptr;
            posix_memalign(&ptr, kAlignment, kAlignment);
            
            chunk = (HeapAllocator *)ptr;
            // Reserve first HEAP_RESERVED_SLOTS slots
            memset(chunk->alloc_map, 0xff, HEAP_RESERVED_SLOTS / 8);
            // Clear the rest of reserved space
            memset(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(chunk->alloc_map) + (HEAP_RESERVED_SLOTS / 8)),
                   0, HEAP_ALLOC_MAP_SIZE * 2 - (HEAP_RESERVED_SLOTS / 8));
            for (int i=0; i<HEAP_SMALL_SPACE_LOG; i++)
                chunk->info.m_small_indicies[i] = HEAP_BLOCKS-1;
            chunk->info.m_large_index = 0;

            chunk->Initialize(heap, reinterpret_cast<internal::Address>(chunk),
                              kAlignment, reinterpret_cast<internal::Address>(chunk), reinterpret_cast<internal::Address>(chunk) - 1,
                              internal::Executability::NOT_EXECUTABLE, nullptr, nullptr);
        }
        assert(alloc || chunk);
    }
    
    memset(alloc, 0, size);

    HeapObject * o = reinterpret_cast<HeapObject*>(alloc);

    if (!map) {
        // We are a map.  Point to ourselves
        map = reinterpret_cast<BaseMap*>(o);
    }
    o->m_map = reinterpret_cast<internal::Map*>(reinterpret_cast<intptr_t>(map) + internal::kHeapObjectTag);

    return o;
}

internal::MemoryChunk* internal::MemoryChunk::Initialize(internal::Heap* heap, internal::Address base, size_t size,
                                                         internal::Address area_start, internal::Address area_end,
                                                         internal::Executability executable, internal::Space* owner,
                                                         base::VirtualMemory* reservation)
{
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(heap);
    MemoryChunk *chunk = reinterpret_cast<MemoryChunk*>(base);
    memset(chunk, 0, sizeof(MemoryChunk));
    chunk->set_next_chunk(heapimpl->m_heap_top);
    heapimpl->m_heap_top = chunk;
    chunk->heap_ = heap;
    return chunk;
}
