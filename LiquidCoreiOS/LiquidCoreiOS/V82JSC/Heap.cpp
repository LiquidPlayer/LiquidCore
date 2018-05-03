//
//  Heap.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/2/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

InternalObjectImpl* HeapAllocator::Alloc(IsolateImpl *isolate, size_t size)
{
    static const size_t kAlignment = 0x20000;
    static const size_t kSlotSize = 0x100;
    static const size_t kReserved = (sizeof(HeapAllocator) + kSlotSize - 1) / kSlotSize;
    
    struct Slot { unsigned char _[kSlotSize]; };
    if (size == 0) size = 1; // Throws off the math otherwise and makes no difference
    
    internal::Heap *heap = reinterpret_cast<internal::Isolate*>(isolate)->heap();
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(heap);
    HeapAllocator *chunk = static_cast<HeapAllocator*>(heapimpl->m_heap_top);
    
    auto find_space = [heapimpl,isolate](HeapAllocator *chunk, size_t num_slots) -> InternalObjectImpl* {
        auto reserve = [heapimpl, isolate](HeapAllocator *chunk, size_t num_slots, size_t start, size_t end) -> InternalObjectImpl* {
            Slot* slots = (Slot*)chunk;
            for (size_t index=start; index < end; ) {
                // Reserve the first kReserved slots for the MemoryChunk
                if (index < kReserved) { index++; continue; }
                InternalObjectImpl *slot = (InternalObjectImpl*) &slots[index];
                bool free = true;
                for(size_t i=0; i<num_slots && free; i++) {
                    InternalObjectImpl *check_slot = (InternalObjectImpl*) &slots[index + i];
                    free = check_slot->m_count == 0;
                }
                if (free) {
                    memset(slot, 0, num_slots * kSlotSize);
                    slot->m_count = 1;
                    slot->m_slots = num_slots;
                    slot->pMap = reinterpret_cast<internal::Map*>(reinterpret_cast<intptr_t>(&slots[index]) + internal::kHeapObjectTag);
                    slot->m_isolate = isolate;
                    heapimpl->m_index = index + num_slots;
                    return reinterpret_cast<InternalObjectImpl*>(&slots[index]);
                } else {
                    index += slot->m_slots;
                }
            }
            return nullptr;
        };

        auto slot = reserve(chunk, num_slots, heapimpl->m_index, (kAlignment / kSlotSize) - (num_slots - 1));
        if (!slot) {
            slot = reserve(chunk, num_slots, 0, heapimpl->m_index);
        }
        return slot;
    };
    
    InternalObjectImpl *alloc = nullptr;
    if (chunk) {
        alloc = find_space(chunk, (size + kSlotSize - 1) / kSlotSize);
    }
    if (!alloc) {
        void *ptr;
        posix_memalign(&ptr, kAlignment, kAlignment);
        memset(ptr, 0, kAlignment);
        
        HeapAllocator *chunk = (HeapAllocator *)ptr;
        chunk->Initialize(heap, reinterpret_cast<internal::Address>(chunk),
                          kAlignment, reinterpret_cast<internal::Address>(chunk), reinterpret_cast<internal::Address>(chunk) - 1,
                          internal::Executability::NOT_EXECUTABLE, nullptr, nullptr);
        alloc = find_space(chunk, (size + kSlotSize - 1) / kSlotSize);
    }
    
    return alloc;
}

internal::MemoryChunk* internal::MemoryChunk::Initialize(internal::Heap* heap, internal::Address base, size_t size,
                                                         internal::Address area_start, internal::Address area_end,
                                                         internal::Executability executable, internal::Space* owner,
                                                         base::VirtualMemory* reservation)
{
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(heap);
    MemoryChunk *chunk = reinterpret_cast<MemoryChunk*>(base);
    chunk->set_next_chunk(heapimpl->m_heap_top);
    heapimpl->m_heap_top = chunk;
    chunk->heap_ = heap;
    return chunk;
}
