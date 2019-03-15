/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef HeapObjects_h
#define HeapObjects_h

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
#include <map>
#include <string>
#include "JSScriptRefPrivate.h"
#include "JSWeakRefPrivate.h"

/* V82JSC Heap Objects are designed to mirror V8 heap objects as much as possible.  Some rules:
 * --> All heap objects have a reference to their v8::internal::Map at offset 0 to identify their type
 * --> heap objects cannot be created with 'new' and 'delete'
 * --> heap objects are fixed in size at creation time and do not expand/contract
 * --> heap objects must not reference data types that require access to the C heap (e.g. std::vector, std:map, etc.)
 * --> heap objects must not contain pointers to other heap objects, only Handle<>s, as they may get moved in memory
 *
 * The basic goal here is to allow the entire heap to be wiped by deallocting it in one go upon isolate destruction
 * without need for further cleanup.  So everything that gets allocated must also be a HeapObject.  Anything that is
 * dynamically allocated with new / malloc are not guaranteed to get destructed and will therefore leak.
 */

#define HEAP_ALIGNMENT (0x80000) // 512K bytes
#define HEAP_SLOT_SIZE_LOG (5)
#define HEAP_SLOT_SIZE (1<<HEAP_SLOT_SIZE_LOG) // 0x20 bytes
#define HEAP_ALLOC_MAP_SIZE (HEAP_ALIGNMENT / (HEAP_SLOT_SIZE * 8)) // 0x800 bytes
#define HEAP_RESERVED_SLOTS ((HEAP_ALLOC_MAP_SIZE * 2) / HEAP_SLOT_SIZE) // 0x40 (64) slots
#define HEAP_SLOTS (HEAP_ALIGNMENT / HEAP_SLOT_SIZE) // 0x4000 (16384)
#define HEAP_SMALL_SPACE_LOG (8) // Up to 64 slots (2K bytes)
#define HEAP_SLOTS_PER_BLOCK (64) // bits in a uint64_t
#define HEAP_BLOCK_SIZE (HEAP_SLOTS_PER_BLOCK * HEAP_SLOT_SIZE) // (2k bytes)
#define HEAP_BLOCKS (HEAP_ALIGNMENT / HEAP_BLOCK_SIZE) // 256

namespace v8 { namespace internal { class IsolateImpl; class SecondPassCallback; } }

namespace V82JSC {
    
using v8::internal::IsolateImpl;

struct HeapObject;
struct BaseMap;
struct FixedArray;

typedef std::map<v8::internal::Object *, int> CanonicalHandles;
typedef std::map<v8::internal::Object *, std::vector<v8::internal::Object**>> WeakHandles;
struct HeapContext {
    CanonicalHandles handles_;
    WeakHandles weak_;
    std::vector<v8::internal::SecondPassCallback> callbacks_;
    std::set<HeapObject*> allocated_;
    std::set<HeapObject*> deallocated_;
};

typedef void (*Constructor)(HeapObject *);
typedef int (*Destructor)(HeapContext&, HeapObject *);

typedef uint8_t (*Transform)(uint64_t);
Transform transform (uint32_t size);
static inline uint32_t LogReserveSize(uint32_t x)
{
    const int kWordSize = sizeof(int) * 8;
    return kWordSize - __builtin_clz((int)((x-1)|1));
}
static inline uint32_t ReserveSize(uint32_t x)
{
    return 1 << LogReserveSize(x);
}
    
class HeapAllocator : public v8::internal::MemoryChunk
{
    struct _info {
        int m_small_indicies[HEAP_SMALL_SPACE_LOG];
        int m_large_index;
        // Anything else we need to store, put here
    } info;
    uint8_t reserved_[HEAP_ALLOC_MAP_SIZE - sizeof(struct _info) - sizeof(v8::internal::MemoryChunk)];
    uint64_t alloc_map[HEAP_ALLOC_MAP_SIZE / 8];
public:
    static HeapObject* Alloc(IsolateImpl *isolate, const BaseMap* map, uint32_t size=0);
    static bool CollectGarbage(IsolateImpl *isolate);
    static int Deallocate(HeapContext&, HeapObject*);
    static void TearDown(IsolateImpl *isolate);
};

struct HeapImpl : v8::internal::Heap {
    v8::internal::MemoryChunk *m_heap_top;
    size_t m_allocated;
};

// All objects on the heap are dervied from HeapObject
struct HeapObject {
    v8::internal::Map *m_map;
    
    IsolateImpl * GetIsolate()
    {
        intptr_t addr = reinterpret_cast<intptr_t>(this);
        addr &= ~(HEAP_ALIGNMENT - 1);
        HeapAllocator *chunk = reinterpret_cast<HeapAllocator*>(addr);
        return reinterpret_cast<IsolateImpl*>(chunk->heap()->isolate());
    }
    
    JSGlobalContextRef GetNullContext();
    
    JSContextGroupRef GetContextGroup();
    
    static int DecrementCount(HeapContext& context, v8::internal::Object *obj)
    {
        assert(obj->IsHeapObject());
        HeapObject *o = reinterpret_cast<HeapObject*>(reinterpret_cast<intptr_t>(obj) - v8::internal::kHeapObjectTag);
        int count = context.handles_[obj];
        assert(count);
        if (-- count == 0) {
            context.handles_.erase(obj);
            if (o->m_map != obj) {
                return HeapAllocator::Deallocate(context, o);
            } else {
                // Don't deallocate maps!
                return 0;
            }
        } else {
            context.handles_[obj] = count;
        }
        return 0;
    }
    
    template <typename T>
    static int SmartReset(HeapContext& context, v8::Persistent<T>& handle)
    {
        if (!handle.IsEmpty()) {
            v8::internal::Object ** persistent = * reinterpret_cast<v8::internal::Object***>(&handle);
            v8::internal::Object *obj = * persistent;
            handle.Reset();
            if (obj->IsHeapObject()) {
                return DecrementCount(context, obj);
            }
        }
        return 0;
    }
};

struct BaseMap /* : HeapObject -- This is implicit */ {
    union {
        HeapObject object;
        unsigned char filler_[v8::internal::Map::kSize];
    };
    uint32_t size;
    Constructor ctor;
    Destructor  dtor;
};

template <typename T>
struct Map : BaseMap
{
    static Map<T> * New(IsolateImpl *iso, v8::internal::InstanceType t, uint8_t kind=0xff);
};

inline v8::internal::Map * ToV8Map(BaseMap *map)
{
    return reinterpret_cast<v8::internal::Map*>(reinterpret_cast<intptr_t>(map) + v8::internal::kHeapObjectTag);
}

inline v8::internal::Object * ToHeapPointer(HeapObject *obj)
{
    return reinterpret_cast<v8::internal::Object*>(reinterpret_cast<intptr_t>(obj) + v8::internal::kHeapObjectTag);
}

inline HeapObject * FromHeapPointer(v8::internal::Object *obj)
{
    return reinterpret_cast<HeapObject*>(reinterpret_cast<intptr_t>(obj) - v8::internal::kHeapObjectTag);
}

template<typename T> Map<T> * Map<T>::New(IsolateImpl *iso, v8::internal::InstanceType t, uint8_t kind)
{
    auto map = reinterpret_cast<Map<T> *>(HeapAllocator::Alloc(iso, nullptr, sizeof(BaseMap)));
    ToV8Map(map)->set_instance_type(t);
    map->ctor = [](HeapObject* o) { T::Constructor(static_cast<T*>(o)); };
    map->dtor = [](HeapContext& context, HeapObject* o) -> int
    {
        return T::Destructor(context, static_cast<T*>(o));
        
    };
    map->size = sizeof(T);
    if (kind != 0xff) {
        v8::internal::Oddball* oddball_handle = reinterpret_cast<v8::internal::Oddball*>(map->object.m_map);
        oddball_handle->set_kind(kind);
        map->size = 0;
    }
    return map;
}
    
} /* namespace V82JSC */

#endif /* HeapObjects_h */
