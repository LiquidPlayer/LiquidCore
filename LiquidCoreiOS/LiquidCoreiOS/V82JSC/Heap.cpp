//
//  Heap.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/2/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

const size_t kSlotSize = 0x80;
static const size_t kReserved = (sizeof(HeapAllocator) + kSlotSize - 1) / kSlotSize;

struct Slot {
    union {
        struct {
            struct {
                int32_t m_count;
                uint32_t m_slots; // This can be uint16_t, but keep alignment
                InternalObjectDestructor m_dtor;
            } header;
            InternalObjectImpl m_io;
        };
        unsigned char _[kSlotSize];
    };
};

InternalObjectImpl* HeapAllocator::Alloc(IsolateImpl *isolate, size_t size, InternalObjectDestructor dtor)
{
    static const size_t kAlignment = 0x80000;

    // Make room for header
    size += sizeof(Slot::header);
    
    internal::Heap *heap = reinterpret_cast<internal::Isolate*>(isolate)->heap();
    HeapImpl *heapimpl = reinterpret_cast<HeapImpl*>(heap);
    HeapAllocator *chunk = static_cast<HeapAllocator*>(heapimpl->m_heap_top);
    
    auto find_space = [heapimpl,isolate,dtor](HeapAllocator *chunk, size_t num_slots) -> InternalObjectImpl* {
        auto reserve = [heapimpl,isolate,dtor](HeapAllocator *chunk, size_t num_slots, size_t start, size_t end) -> InternalObjectImpl* {
            Slot* slots = (Slot*)chunk;
            for (size_t index=start; index < end; ) {
                // Reserve the first kReserved slots for the MemoryChunk
                if (index < kReserved) { index++; continue; }
                Slot *slot = &slots[index];
                bool free = true;
                for(size_t i=0; i<num_slots && free; i++) {
                    Slot *check_slot = &slots[index + i];
                    free = check_slot->header.m_count == -1;
                }
                if (free) {
                    memset(slot, 0, num_slots * kSlotSize);
                    slot->header.m_count = 0;
                    slot->header.m_slots = (uint32_t) num_slots;
                    slot->m_io.pMap = reinterpret_cast<internal::Map*>(reinterpret_cast<intptr_t>(&slot->m_io) + internal::kHeapObjectTag);
                    slot->header.m_dtor = dtor;
                    heapimpl->m_index = index + num_slots;
                    return &slot->m_io;
                } else {
                    index += slot->header.m_slots;
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
        memset(ptr, 0xff, kAlignment);  // All space will be deallocated (m_count = -1)
        
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
    memset(chunk, 0, sizeof(MemoryChunk));
    chunk->set_next_chunk(heapimpl->m_heap_top);
    heapimpl->m_heap_top = chunk;
    chunk->heap_ = heap;
    return chunk;
}

static const intptr_t io_offset = reinterpret_cast<intptr_t>(&reinterpret_cast<Slot*>(16)->m_io) - 16;

void InternalObjectImpl::Retain()
{
    Slot* slot = reinterpret_cast<Slot*>(reinterpret_cast<intptr_t>(this) - io_offset);
    slot->header.m_count ++;
}

void InternalObjectImpl::Release()
{
    Slot* slot = reinterpret_cast<Slot*>(reinterpret_cast<intptr_t>(this) - io_offset);
    assert(slot->header.m_count >= 0);
    if (--slot->header.m_count == -1 && slot->header.m_dtor) {
        slot->header.m_dtor(this);
    }
}
