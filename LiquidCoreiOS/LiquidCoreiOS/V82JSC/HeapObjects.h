//
//  HeapObjects.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 5/27/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

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

#define Copyable(T) \
v8::Persistent<T, v8::CopyablePersistentTraits<T>>

// These aren't really V8 values, but we want to use V8 handles to manage their
// lifecycle, so we pretend.
namespace v8 {
    struct PropAccessor : v8::Value {};
    struct Prop : v8::Value {};
    struct ObjAccessor : v8::Value {};
    struct IntrinsicProp : v8::Value {};
    struct EmbeddedFixedArray : v8::Value {};
    struct TrackedObject : v8::Value {};
    struct AccessorInfo : v8::Value {};
};

namespace V82JSC_HeapObject {
    using v8::internal::IsolateImpl;
    
    struct HeapObject;
    struct BaseMap;
    struct FixedArray;
    
    typedef std::map<v8::internal::Object *, int> CanonicalHandles;
    typedef std::map<v8::internal::Object *, std::vector<v8::internal::Object**>> WeakHandles;
    typedef void (*Constructor)(HeapObject *);
    typedef int (*Destructor)(HeapObject *, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>&);
    typedef void (*Mover)(HeapObject *from, HeapObject *to);
    
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
        static int Deallocate(HeapObject*, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks);
        static void TearDown(IsolateImpl *isolate);
    };
    
    struct HeapImpl : v8::internal::Heap {
        v8::internal::MemoryChunk *m_heap_top;
        size_t m_index;
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
        static int DecrementCount(v8::internal::Object *obj, CanonicalHandles& handles, WeakHandles& weak,
                                  std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            assert(handles.count(obj));
            int count = handles[obj];
            if (-- count == 0) {
                handles.erase(obj);
                HeapObject *o = reinterpret_cast<HeapObject*>(reinterpret_cast<intptr_t>(obj) - v8::internal::kHeapObjectTag);
                if (o->m_map != obj) {
                    return HeapAllocator::Deallocate(o, handles, weak, callbacks);
                } else {
                    // Don't deallocate maps!
                    return 0;
                }
            } else {
                handles[obj] = count;
            }
            return 0;
        }
        template <typename T>
        static int SmartReset(Copyable(T)& handle, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            if (!handle.IsEmpty()) {
                v8::internal::Object ** persistent = * reinterpret_cast<v8::internal::Object***>(&handle);
                v8::internal::Object *obj = * persistent;
                handle.Reset();
                if (obj->IsHeapObject()) {
                    return DecrementCount(obj, handles, weak, callbacks);
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
        Mover       mover;
    };
    
    template <typename T>
    struct Map : BaseMap
    {
        static Map<T> * New(IsolateImpl *iso, v8::internal::InstanceType t, uint8_t kind=0xff);
    };
    
    struct Context : HeapObject {
        uint8_t reserved_[v8::internal::Internals::kContextHeaderSize +
                          ((v8::internal::kApiPointerSize + 1) * v8::internal::Internals::kContextEmbedderDataIndex)
                          - sizeof(HeapObject)];
        JSContextRef m_ctxRef;
        
        static void Constructor(Context *obj) {}
        static int Destructor(Context *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            return 0;
        }
    };
    
    struct GlobalContext : Context {
        Copyable(v8::EmbeddedFixedArray) m_embedder_data;
        Copyable(v8::Function) ObjectSetPrototypeOf;
        Copyable(v8::Function) ObjectGetPrototypeOf;
        Copyable(v8::Function) ObjectPrototypeToString;
        Copyable(v8::Function) FunctionPrototypeBind;

        static void Constructor(GlobalContext *obj) {}
        static int Destructor(GlobalContext *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            IsolateImpl *iso = obj->GetIsolate();
            
            if (obj->m_ctxRef) JSGlobalContextRelease((JSGlobalContextRef)obj->m_ctxRef);
            int freed=0;
            freed +=SmartReset<v8::Function>(obj->ObjectSetPrototypeOf, handles, weak, callbacks);
            freed +=SmartReset<v8::Function>(obj->ObjectGetPrototypeOf, handles, weak, callbacks);
            freed +=SmartReset<v8::Function>(obj->ObjectPrototypeToString, handles, weak, callbacks);
            freed +=SmartReset<v8::Function>(obj->FunctionPrototypeBind, handles, weak, callbacks);
            freed +=SmartReset<v8::EmbeddedFixedArray>(obj->m_embedder_data, handles, weak, callbacks);

            RemoveContextFromIsolate(iso, (JSGlobalContextRef)obj->m_ctxRef);
            
            return freed + Context::Destructor(obj, handles, weak, callbacks);
        }
        static void RemoveContextFromIsolate(IsolateImpl* iso, JSGlobalContextRef ctx);
    };
    
    struct UnboundScript : HeapObject {
        JSScriptRef m_script;
        JSStringRef m_script_string;
        Copyable(v8::Integer) m_id;
        Copyable(v8::Value) m_resource_name;
        Copyable(v8::Value) m_sourceURL;
        Copyable(v8::Value) m_sourceMappingURL;
        Copyable(v8::Integer) m_resource_line_offset;
        Copyable(v8::Integer) m_resource_column_offset;
        bool m_resource_is_shared_cross_origin;
        bool m_resource_is_opaque;
        bool m_is_wasm;
        bool m_is_module;

        static void Constructor(UnboundScript *obj) {}
        static int Destructor(UnboundScript *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            if (obj->m_script) JSScriptRelease(obj->m_script);
            if (obj->m_script_string) JSStringRelease(obj->m_script_string);
            
            int freed=0;
            freed +=SmartReset<v8::Value>(obj->m_resource_name, handles, weak, callbacks);
            freed +=SmartReset<v8::Value>(obj->m_sourceURL, handles, weak, callbacks);
            freed +=SmartReset<v8::Value>(obj->m_sourceMappingURL, handles, weak, callbacks);
            freed +=SmartReset<v8::Integer>(obj->m_resource_line_offset, handles, weak, callbacks);
            freed +=SmartReset<v8::Integer>(obj->m_resource_column_offset, handles, weak, callbacks);
            freed +=SmartReset<v8::Integer>(obj->m_id, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct Script : HeapObject {
        Copyable(v8::UnboundScript) m_unbound_script;
        Copyable(v8::Context) m_context;

        static void Constructor(Script *obj) {}
        static int Destructor(Script *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::UnboundScript>(obj->m_unbound_script, handles, weak, callbacks);
            freed +=SmartReset<v8::Context>(obj->m_context, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct Value : HeapObject {
        uint64_t reserved_; // For number, value is stored here
        JSValueRef m_value;
        uint64_t reserved2_; // For string, resource is stored here

        static void Constructor(Value *obj) {}
        static int Destructor(Value *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            if (obj->m_value) JSValueUnprotect(obj->GetNullContext(), obj->m_value);
            return 0;
        }
    };

    struct WeakValue : Value {
        static void Constructor(WeakValue *obj)
        {
            Value::Constructor(obj);
        }
        static int Destructor(WeakValue *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            // Don't call value destructor
            return 0;
        }
    };

    struct String : Value {
        static void Constructor(String *obj)
        {
            Value::Constructor(obj);
        }
        static int Destructor(String *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            return Value::Destructor(obj, handles, weak, callbacks);
        }
    };
    
    struct FixedArray : HeapObject {
        union {
            uint8_t __buffer[v8::internal::Internals::kFixedArrayHeaderSize - sizeof(HeapObject)];
            int m_size;
        };
        v8::internal::Object* m_elements[0];

        static void Constructor(FixedArray *obj) {}
        static int Destructor(FixedArray *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            for (int i=0; i<obj->m_size; i++) {
                if (obj->m_elements[i]->IsHeapObject()) {
                    freed += DecrementCount(obj->m_elements[i], handles, weak, callbacks);
                }
            }
            return freed;
        }
    };
    
    struct PropAccessor : HeapObject {
        Copyable(v8::Name) name;
        Copyable(v8::FunctionTemplate) setter;
        Copyable(v8::FunctionTemplate) getter;
        v8::PropertyAttribute attribute;
        v8::AccessControl settings;
        Copyable(v8::PropAccessor) next_;

        static void Constructor(PropAccessor *obj) {}
        static int Destructor(PropAccessor *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::Name>(obj->name, handles, weak, callbacks);
            freed +=SmartReset<v8::FunctionTemplate>(obj->setter, handles, weak, callbacks);
            freed +=SmartReset<v8::FunctionTemplate>(obj->getter, handles, weak, callbacks);
            freed +=SmartReset<v8::PropAccessor>(obj->next_, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct Prop : HeapObject {
        Copyable(v8::Name) name;
        Copyable(v8::Data) value;
        v8::PropertyAttribute attributes;
        Copyable(v8::Prop) next_;

        static void Constructor(Prop *obj) {}
        static int Destructor(Prop *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::Name>(obj->name, handles, weak, callbacks);
            freed +=SmartReset<v8::Data>(obj->value, handles, weak, callbacks);
            freed +=SmartReset<v8::Prop>(obj->next_, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct ObjAccessor : HeapObject {
        Copyable(v8::Name) name;
        Copyable(v8::Value) data;
        Copyable(v8::Signature) signature;
        v8::AccessorNameGetterCallback getter;
        v8::AccessorNameSetterCallback setter;
        v8::AccessControl settings;
        v8::PropertyAttribute attribute;
        Copyable(v8::ObjAccessor) next_;

        static void Constructor(ObjAccessor *obj) {}
        static int Destructor(ObjAccessor *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::Name>(obj->name, handles, weak, callbacks);
            freed +=SmartReset<v8::Value>(obj->data, handles, weak, callbacks);
            freed +=SmartReset<v8::Signature>(obj->signature, handles, weak, callbacks);
            freed +=SmartReset<v8::ObjAccessor>(obj->next_, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct IntrinsicProp : HeapObject {
        Copyable(v8::Name) name;
        v8::Intrinsic value;
        Copyable(v8::IntrinsicProp) next_;

        static void Constructor(IntrinsicProp *obj) {}
        static int Destructor(IntrinsicProp *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::Name>(obj->name, handles, weak, callbacks);
            freed +=SmartReset<v8::IntrinsicProp>(obj->next_, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct Template : HeapObject {
        JSValueRef m_data;
        Copyable(v8::Prop) m_properties;
        Copyable(v8::PropAccessor) m_property_accessors;
        Copyable(v8::ObjAccessor) m_accessors;
        Copyable(v8::IntrinsicProp) m_intrinsics;
        Copyable(v8::Signature) m_signature;
        Copyable(v8::ObjectTemplate) m_prototype_template;
        Copyable(v8::FunctionTemplate) m_parent;
        v8::FunctionCallback m_callback;

        static void Constructor(Template *obj) {}
        static int Destructor(Template *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::Prop>(obj->m_properties, handles, weak, callbacks);
            freed +=SmartReset<v8::PropAccessor>(obj->m_property_accessors, handles, weak, callbacks);
            freed +=SmartReset<v8::ObjAccessor>(obj->m_accessors, handles, weak, callbacks);
            freed +=SmartReset<v8::IntrinsicProp>(obj->m_intrinsics, handles, weak, callbacks);
            freed +=SmartReset<v8::Signature>(obj->m_signature, handles, weak, callbacks);
            freed +=SmartReset<v8::ObjectTemplate>(obj->m_prototype_template, handles, weak, callbacks);
            freed +=SmartReset<v8::FunctionTemplate>(obj->m_parent, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct FunctionTemplate : Template {
        Copyable(v8::ObjectTemplate) m_instance_template;
        v8::ConstructorBehavior m_behavior;
        Copyable(v8::String) m_name;
        JSObjectRef m_functions_array;
        int m_length;
        bool m_isHiddenPrototype;
        bool m_removePrototype;
        bool m_readOnlyPrototype;

        static void Constructor(FunctionTemplate *obj) { Template::Constructor(obj); }
        static int Destructor(FunctionTemplate *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed +=SmartReset<v8::ObjectTemplate>(obj->m_instance_template, handles, weak, callbacks);
            freed +=SmartReset<v8::String>(obj->m_name, handles, weak, callbacks);
            if (obj->m_functions_array) JSValueUnprotect(obj->GetNullContext(), obj->m_functions_array);
            return freed + Template::Destructor(obj, handles, weak, callbacks);
        }
    };
    
    struct ObjectTemplate : Template {
        JSValueRef m_named_data;
        JSValueRef m_indexed_data;
        JSValueRef m_access_check_data;
        JSValueRef m_failed_named_data;
        JSValueRef m_failed_indexed_data;
        Copyable(v8::FunctionTemplate) m_constructor_template;
        v8::NamedPropertyHandlerConfiguration m_named_handler;
        v8::IndexedPropertyHandlerConfiguration m_indexed_handler;
        v8::AccessCheckCallback m_access_check;
        v8::NamedPropertyHandlerConfiguration m_named_failed_access_handler;
        v8::IndexedPropertyHandlerConfiguration m_indexed_failed_access_handler;
        bool m_need_proxy;
        int m_internal_fields;

        static void Constructor(ObjectTemplate *obj) { Template::Constructor(obj); }
        static int Destructor(ObjectTemplate *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed += SmartReset<v8::FunctionTemplate>(obj->m_constructor_template, handles, weak, callbacks);
            if (obj->m_named_data) JSValueUnprotect(obj->GetNullContext(), obj->m_named_data);
            if (obj->m_indexed_data) JSValueUnprotect(obj->GetNullContext(), obj->m_indexed_data);
            if (obj->m_access_check_data) JSValueUnprotect(obj->GetNullContext(), obj->m_access_check_data);
            if (obj->m_failed_named_data) JSValueUnprotect(obj->GetNullContext(), obj->m_failed_named_data);
            if (obj->m_failed_indexed_data) JSValueUnprotect(obj->GetNullContext(), obj->m_failed_indexed_data);
            return freed + Template::Destructor(obj, handles, weak, callbacks);
        }
    };

    struct TrackedObject : HeapObject {
        JSValueRef m_security;
        JSValueRef m_proxy_security;
        JSValueRef m_hidden_proxy_security;
        JSValueRef m_private_properties;
        JSObjectRef m_hidden_children_array;
        int m_num_internal_fields;
        JSObjectRef m_internal_fields_array;
        Copyable(v8::ObjectTemplate) m_object_template;
        int m_hash;
        bool m_isHiddenPrototype;
        bool m_isGlobalObject;
        void *m_embedder_data[2];

        struct {
            void *buffer;
            size_t byte_length;
            bool isExternal;
            bool isNeuterable;
            IsolateImpl *iso;
            Copyable(v8::TrackedObject) m_self;
        } ArrayBufferInfo;

        static void Constructor(TrackedObject *obj) {}
        static int Destructor(TrackedObject *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed += SmartReset<v8::ObjectTemplate>(obj->m_object_template, handles, weak, callbacks);
            // obj->m_security is a weak reference to avoid circular referencing
            if (obj->m_proxy_security) JSValueUnprotect(obj->GetNullContext(), obj->m_proxy_security);
            if (obj->m_hidden_proxy_security) JSValueUnprotect(obj->GetNullContext(), obj->m_hidden_proxy_security);
            if (obj->m_private_properties) JSValueUnprotect(obj->GetNullContext(), obj->m_private_properties);
            if (obj->m_hidden_children_array) JSValueUnprotect(obj->GetNullContext(), obj->m_hidden_children_array);
            if (obj->m_internal_fields_array) JSValueUnprotect(obj->GetNullContext(), obj->m_internal_fields_array);
            return freed;
        }
    };
    
    struct Signature : HeapObject {
        Copyable(v8::FunctionTemplate) m_template;

        static void Constructor(Signature *obj) {}
        static int Destructor(Signature *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            return SmartReset<v8::FunctionTemplate>(obj->m_template, handles, weak, callbacks);
        }
    };
    
    struct Accessor : HeapObject {
        JSValueRef m_property;
        JSValueRef m_data;
        JSValueRef m_holder;
        v8::AccessorNameGetterCallback getter;
        v8::AccessorNameSetterCallback setter;

        static void Constructor(Accessor *obj) {}
        static int Destructor(Accessor *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            if (obj->m_property) JSValueUnprotect(obj->GetNullContext(), obj->m_property);
            if (obj->m_data) JSValueUnprotect(obj->GetNullContext(), obj->m_data);
            if (obj->m_holder) JSValueUnprotect(obj->GetNullContext(), obj->m_holder);
            return 0;
        }
    };
    
    struct Message : Value {
        Copyable(v8::Script) m_script;
        JSStringRef m_back_trace;

        static void Constructor(Message *obj) {}
        static int Destructor(Message *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            if (obj->m_back_trace) JSStringRelease(obj->m_back_trace);
            int freed=0;
            freed += SmartReset<v8::Script>(obj->m_script, handles, weak, callbacks);
            return freed + Value::Destructor(obj, handles, weak, callbacks);
        }
    };
    
    struct StackTrace : HeapObject {
        Copyable(v8::Script) m_script;
        JSObjectRef m_error;
        JSObjectRef m_stack_frame_array;
        
        static void Constructor(StackTrace *obj) {}
        static int Destructor(StackTrace *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            if (obj->m_error) JSValueUnprotect(obj->GetNullContext(), obj->m_error);
            if (obj->m_stack_frame_array) JSValueUnprotect(obj->GetNullContext(), obj->m_stack_frame_array);
            int freed=0;
            freed += SmartReset<v8::Script>(obj->m_script, handles, weak, callbacks);
            return freed;
        }
    };
    
    struct StackFrame : HeapObject {
        Copyable(v8::String) m_function_name;
        Copyable(v8::String) m_script_name;
        Copyable(v8::StackTrace) m_stack_trace;
        int m_line_number;
        int m_column_number;
        bool m_is_eval;
        
        static void Constructor(StackFrame *obj) {}
        static int Destructor(StackFrame *obj, CanonicalHandles& handles, WeakHandles& weak,
                              std::vector<v8::internal::SecondPassCallback>& callbacks)
        {
            int freed=0;
            freed += SmartReset<v8::String>(obj->m_function_name, handles, weak, callbacks);
            freed += SmartReset<v8::String>(obj->m_script_name, handles, weak, callbacks);
            freed += SmartReset<v8::StackTrace>(obj->m_stack_trace, handles, weak, callbacks);
            return freed;
        }
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
        map->dtor = [](HeapObject* o, CanonicalHandles& handles, WeakHandles& weak,
                       std::vector<v8::internal::SecondPassCallback>& callbacks) -> int
        {
            return T::Destructor(static_cast<T*>(o), handles, weak, callbacks);
            
        };
        map->size = sizeof(T);
        if (kind != 0xff) {
            v8::internal::Oddball* oddball_handle = reinterpret_cast<v8::internal::Oddball*>(map->object.m_map);
            oddball_handle->set_kind(kind);
            map->size = 0;
        }
        return map;
    }
}

#endif /* HeapObjects_h */
