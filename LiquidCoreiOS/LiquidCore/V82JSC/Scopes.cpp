/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSCPrivate.h"
#include "Object.h"
#include <malloc/malloc.h>

using namespace V82JSC;
using namespace v8;

#define HANDLEBLOCK_SIZE 0x4000
#define NUM_HANDLES ((HANDLEBLOCK_SIZE - 2*sizeof(HandleBlock*))/ sizeof(internal::Object*))
struct HandleBlock {
    HandleBlock * previous_;
    HandleBlock * next_;
    internal::Object * handles_[NUM_HANDLES];
};

static const uint8_t kActiveWeak = internal::Internals::kNodeStateIsWeakValue +
    (1 << internal::Internals::kNodeIsActiveShift);
static const uint8_t kActiveWeakMask = internal::Internals::kNodeStateMask +
    (1 << internal::Internals::kNodeIsActiveShift);
static const uint8_t kActiveNearDeath = internal::Internals::kNodeStateIsNearDeathValue +
    (1 << internal::Internals::kNodeIsActiveShift);

HandleScope::HandleScope(Isolate* isolate)
{
    IsolateImpl::PerThreadData::EnterThreadContext(isolate);
    Initialize(isolate);
}

void HandleScope::Initialize(Isolate* isolate)
{
    IsolateImpl* impl = ToIsolateImpl(isolate);
    
    isolate_ = reinterpret_cast<internal::Isolate*>(isolate);
    prev_next_ = impl->ii.handle_scope_data()->next;
    prev_limit_ = impl->ii.handle_scope_data()->limit;
    
    impl->m_scope_stack->push_back(this);
}


void delBlock(HandleBlock *block)
{
    if (block && block->next_) delBlock(block->next_);
    free (block);
}

HandleScope::~HandleScope()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(isolate_);
    internal::HandleScopeData *data = impl->ii.handle_scope_data();

    assert(this == impl->m_scope_stack->back());
    data->next = prev_next_;
    data->limit = prev_limit_;
    
    // Clear any HandleBlocks that are done
    if (data->limit) {
        intptr_t addr = reinterpret_cast<intptr_t>(data->limit - 1);
        addr &= ~(HANDLEBLOCK_SIZE-1);
        HandleBlock *thisBlock = reinterpret_cast<HandleBlock*>(addr);
        delBlock(thisBlock->next_);
        thisBlock->next_ = nullptr;
        if (impl->m_scope_stack->size() == 1) {
            CHECK_EQ(data->next, data->limit);
            free (thisBlock);
            data->next = nullptr;
            data->limit = nullptr;
        }
    }
    
    impl->m_scope_stack->pop_back();
}

//
// internal::HandleScope
//
// Deallocates any extensions used by the current scope.
void internal::HandleScope::DeleteExtensions(Isolate* isolate)
{
    assert(0);
}

// Counts the number of allocated handles.
int internal::HandleScope::NumberOfHandles(Isolate* ii)
{
    // Seal off current thread's handle scopes
    auto thread = IsolateImpl::PerThreadData::Get(reinterpret_cast<IsolateImpl*>(ii));
    memcpy(&thread->m_handle_scope_data, ii->handle_scope_data(), sizeof(HandleScopeData));

    size_t handles = 0;
    HandleScopeData *data = ii->handle_scope_data();
    if (data->limit) {
        intptr_t addr = reinterpret_cast<intptr_t>(data->limit - 1);
        addr &= ~(HANDLEBLOCK_SIZE -1);
        HandleBlock *block = reinterpret_cast<HandleBlock*>(addr);
        if (block) {
            handles = data->next - &block->handles_[0];
            block = block->previous_;
        }
        while (block) {
            handles += NUM_HANDLES;
            block = block->previous_;
        }
    }
    return (int)handles;
}

// Extend the handle scope making room for more handles.
internal::Object** internal::HandleScope::Extend(Isolate* isolate)
{
    IsolateImpl* isolateimpl = reinterpret_cast<IsolateImpl*>(isolate);
    DCHECK(!isolateimpl->m_scope_stack->empty());
    
    HandleBlock *ptr;
    posix_memalign((void**)&ptr, HANDLEBLOCK_SIZE, sizeof(HandleBlock));
    
    internal::Object **handles = &ptr->handles_[0];
    HandleScopeData *data = isolateimpl->ii.handle_scope_data();

    if (data->next != nullptr) {
        intptr_t addr = reinterpret_cast<intptr_t>(data->limit - 1);
        addr &= ~(HANDLEBLOCK_SIZE - 1);
        ptr->previous_ = reinterpret_cast<HandleBlock*>(addr);
        ptr->previous_->next_ = ptr;
    } else {
        ptr->previous_ = nullptr;
    }
    ptr->next_ = nullptr;

    data->next = handles;
    data->limit = &handles[NUM_HANDLES];

    return handles;
}

void IsolateImpl::GetActiveLocalHandles(HeapContext& context)
{
    // Seal off current thread's handle scopes
    auto thread = PerThreadData::Get(this);
    memcpy(&thread->m_handle_scope_data, ii.handle_scope_data(), sizeof(HandleScopeData));
    
    std::unique_lock<std::mutex> lock(s_thread_data_mutex);
    for (auto it=s_thread_data.begin(); it!=s_thread_data.end(); it++) {
        for (auto it2=it->second->m_isolate_data.begin(); it2!=it->second->m_isolate_data.end(); it2++) {
            if (it2->first == this) {
                HandleScopeData *data = &it2->second->m_handle_scope_data;
                if (data->limit) {
                    intptr_t addr = reinterpret_cast<intptr_t>(data->limit - 1);
                    addr &= ~(HANDLEBLOCK_SIZE -1);
                    HandleBlock *block = reinterpret_cast<HandleBlock*>(addr);
                    internal::Object ** limit = data->next;
                    while (block) {
                        for (internal::Object ** handle = &block->handles_[0]; handle < limit; handle++ ) {
                            if ((*handle)->IsHeapObject()) {
                                int count = context.handles_.count(*handle) ? context.handles_[*handle] + 1: 1;
                                context.handles_[*handle] = count;
                            }
                        }
                        block = block->next_;
                        limit = block ? &block->next_->handles_[NUM_HANDLES] : nullptr;
                    }
                }
            }
        }
    }
}

internal::Object** internal::CanonicalHandleScope::Lookup(Object* object)
{
    assert(0);
    return nullptr;
}

int HandleScope::NumberOfHandles(Isolate* isolate)
{
    return internal::HandleScope::NumberOfHandles(reinterpret_cast<internal::Isolate*>(isolate));
}

internal::Object** HandleScope::CreateHandle(internal::Isolate* isolate,
                                       internal::Object* value)
{
    IsolateImpl* impl = ToIsolateImpl(reinterpret_cast<Isolate*>(isolate));
    return internal::HandleScope::CreateHandle(&impl->ii, value);
}

internal::Object** HandleScope::CreateHandle(internal::HeapObject* heap_object,
                                       internal::Object* value)
{
    return CreateHandle(heap_object->GetIsolate(), value);
}

EscapableHandleScope::EscapableHandleScope(Isolate* isolate) : HandleScope()
{
    IsolateImpl::PerThreadData::EnterThreadContext(isolate);
    // Creates a slot which we will transfer to the previous scope upon Escape()
    escape_slot_ = HandleScope::CreateHandle(reinterpret_cast<internal::Isolate*>(isolate), 0);
    Initialize(isolate);
}

internal::Object** EscapableHandleScope::Escape(internal::Object** escape_value)
{
    if (escape_value) {
        *escape_slot_ = *escape_value;
        return escape_slot_;
    } else {
        return nullptr;
    }
}

SealHandleScope::~SealHandleScope()
{
    
}

SealHandleScope::SealHandleScope(Isolate* isolate) : isolate_(nullptr)
{
    
}

class internal::GlobalHandles::Node {
public:
    internal::Object *handle_;
    uint16_t classId_; // internal::Internals::kNodeClassIdOffset (1 *kApiPointerSize)
    uint8_t  index_;
    uint8_t  flags_;   // internal::Internals::kNodeFlagsOffset (1 * kApiPointerSize + 3)
    uint8_t  reserved_[internal::kApiPointerSize - sizeof(uint32_t)]; // align
    void *embedder_fields_[2];
    v8::WeakCallbackType type_;
    WeakCallbackInfo<void>::Callback weak_callback_;
    WeakCallbackInfo<void>::Callback second_pass_callback_;
    void *param_;
};

class internal::GlobalHandles::NodeBlock {
public:
    bool IsEmpty() { return bitmap_ == 0xffffffffffffffff; }
    bool IsFull() { return bitmap_ == 0; }
    int UsedSlots() { return 64 - __builtin_popcountll(bitmap_); }
    bool InAvailableList() { return next_block_ || prev_block_ || global_handles_->first_block_ == this; }
    bool InUsedList() { return next_used_block_ || prev_used_block_ || global_handles_->first_used_block_ == this; }
    void remove_used() {
        IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(global_handles_->isolate());
        std::lock_guard<std::mutex> lock(iso->m_handlewalk_lock);

        assert(!InAvailableList());
        assert(InUsedList());
        assert(UsedSlots() == 63);
        
        if (global_handles_->first_used_block_ == this) {
            assert(prev_used_block_ == nullptr);
            global_handles_->first_used_block_ = next_used_block_;
        }
        if (next_used_block_) {
            assert(next_used_block_->prev_used_block_ == this);
            next_used_block_->prev_used_block_ = prev_used_block_;
        }
        if (prev_used_block_) {
            assert(prev_used_block_->next_used_block_ == this);
            prev_used_block_->next_used_block_ = next_used_block_;
        }
        next_used_block_ = nullptr;
        prev_used_block_ = nullptr;
        assert(!InUsedList());
    }
    void push_used() {
        IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(global_handles_->isolate());
        std::lock_guard<std::mutex> lock(iso->m_handlewalk_lock);

        assert(!InAvailableList());
        assert(!InUsedList());
        assert(IsFull());

        next_used_block_ = global_handles_->first_used_block_;
        if (next_used_block_) {
            next_used_block_->prev_used_block_ = this;
        }
        global_handles_->first_used_block_ = this;
        assert(InUsedList());
    }
    void remove_available() {
        IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(global_handles_->isolate());
        std::lock_guard<std::mutex> lock(iso->m_handlewalk_lock);

        assert(!InUsedList());
        assert(InAvailableList());
        assert(IsEmpty() || IsFull());

        if (global_handles_->first_block_ == this) {
            assert(prev_block_ == nullptr);
            global_handles_->first_block_ = next_block_;
        }
        if (next_block_) {
            assert(next_block_->prev_block_ == this);
            next_block_->prev_block_ = prev_block_;
        }
        if (prev_block_) {
            assert(prev_block_->next_block_ == this);
            prev_block_->next_block_ = next_block_;
        }
        next_block_ = nullptr;
        prev_block_ = nullptr;
        assert(!InAvailableList());
    }
    void push_available() {
        IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(global_handles_->isolate());
        std::lock_guard<std::mutex> lock(iso->m_handlewalk_lock);

        assert(!InAvailableList());
        assert(!InUsedList());
        assert(UsedSlots() == 63);

        next_block_ = global_handles_->first_block_;
        if (next_block_) {
            next_block_->prev_block_ = this;
        }
        global_handles_->first_block_ = this;
        assert(InAvailableList());
    }
    NodeBlock(GlobalHandles *global_handles) {
        global_handles_ = global_handles;
        next_used_block_ = nullptr;
        prev_used_block_ = nullptr;
        next_block_ = nullptr;
        prev_block_ = nullptr;
        bitmap_ = 0xffffffffffffffff;
    }
    internal::Object ** New(internal::Object *value)
    {
        assert (!IsFull());
        global_handles_->number_of_global_handles_ ++;
        int rightmost_set_bit_index = ffsll(bitmap_) - 1;
        uint64_t mask = (uint64_t)1 << rightmost_set_bit_index;
        assert((bitmap_ & mask) == mask);
        bitmap_ &= ~mask;
        if (IsFull()) {
            // Remove from available pool and add to used list
            remove_available();
            push_used();
        }
        memset(&handles_[rightmost_set_bit_index], 0, sizeof(Node));
        handles_[rightmost_set_bit_index].index_ = (int) rightmost_set_bit_index;
        handles_[rightmost_set_bit_index].handle_ = value;
        return &handles_[rightmost_set_bit_index].handle_;
    }
    void Reset(internal::Object ** handle)
    {
        // A wayward reset may come in after the memory has been cleared.  Just
        // ignore it.
        if (malloc_size(this) == 0) return;

        global_handles_->number_of_global_handles_ --;

        int64_t index = (reinterpret_cast<intptr_t>(handle) - reinterpret_cast<intptr_t>(&handles_[0])) / sizeof(Node);
        assert(index >= 0 && index < 64);
        bool wasFull = IsFull();
        uint64_t mask = (uint64_t)1 << index;
        assert((bitmap_ & mask) == 0);
        bitmap_ |= mask;
        if (IsEmpty()) {
            // NodeBlock is empty, remove from available list and delete
            remove_available();
            delete this;
        } else if (wasFull) {
            // Remove from used list and add to avaialable pool
            remove_used();
            push_available();
        }
    }
    int CollectHandles(HeapContext& context) {
        assert((InAvailableList() && !InUsedList()) || (InUsedList() && !InAvailableList()));
        int handles_processed = 0;
        uint64_t mask = 1;
        for (int i=0; i<64; i++) {
            if (~(bitmap_) & mask) {
                Node& node = handles_[i];
                internal::Object *h = node.handle_;
                IsolateImpl* iso = reinterpret_cast<IsolateImpl*>(global_handles_->isolate());
                if (h->IsHeapObject()) {
                    if ((node.flags_ & kActiveWeakMask) == internal::Internals::kNodeStateIsNearDeathValue ||
                        (node.flags_ & kActiveWeakMask) == internal::Internals::kNodeStateIsWeakValue) {
                        
                        auto obj = FromHeapPointer(h);
                        BaseMap *map =
                        reinterpret_cast<BaseMap*>(FromHeapPointer(obj->m_map));
                        if (h->IsPrimitive() || (map != iso->m_value_map && map != iso->m_array_buffer_map)) {
                            if (context.weak_.count(h)) context.weak_[h].push_back(&node.handle_);
                            else {
                                auto vector = std::vector<internal::Object**>();
                                vector.push_back(&node.handle_);
                                context.weak_[h] = std::move(vector);
                            }
                        }
                    } else {
                        int count = context.handles_.count(h) ? context.handles_[h] + 1: 1;
                        context.handles_[h] = count;
                    }
                }
                handles_processed ++;
            }
            mask <<= 1;
        }
        return handles_processed;
    }
    void PerformIncrementalMarking(JSCPrivate::JSMarkerRef marker, std::map<void*, JSObjectRef>& ready_to_die)
    {
        uint64_t mask = 1;
        for (int i=0; i<64; i++) {
            if (~(bitmap_) & mask) {
                Node& node = handles_[i];
                internal::Object *h = node.handle_;
                if (h->IsHeapObject()) {
                    if ((node.flags_ & kActiveWeakMask) == kActiveWeak) {
                        auto vi = ToImpl<V82JSC::Value>(&node.handle_);
                        if (!marker->IsMarked(marker, (JSObjectRef)vi->m_value)) {
                            node.flags_ = (node.flags_ & ~kActiveWeakMask) | kActiveNearDeath;
                            ready_to_die[&node] = (JSObjectRef) vi->m_value;
                        }
                    }
                }
            }
            mask <<= 1;
        }
    }
    
private:
    NodeBlock *next_block_;
    NodeBlock *prev_block_;
    NodeBlock *next_used_block_;
    NodeBlock *prev_used_block_;
    GlobalHandles *global_handles_;
    std::mutex mutex;
    uint64_t bitmap_;
    Node handles_[64];
    friend class internal::GlobalHandles;
    friend class IsolateImpl;
};

internal::GlobalHandles::GlobalHandles(internal::Isolate *isolate)
{
    isolate_ = isolate;
    number_of_global_handles_ = 0;
    first_block_ = nullptr;
    first_used_block_ = nullptr;
    first_free_ = nullptr;

    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(isolate);
    iso->getGlobalHandles = [iso, this](HeapContext& context)
    {
        std::lock_guard<std::mutex> lock(iso->m_handlewalk_lock);
        int total_processed = 0;
        for (NodeBlock *block = first_used_block_; block; block = block->next_used_block_) {
            assert(block->IsFull());
            int processed = block->CollectHandles(context);
            assert(processed == 64);
            total_processed += processed;
        }
        int used_processed = total_processed;
        for (NodeBlock *block = first_block_; block; block = block->next_block_) {
            assert(!block->IsFull() && !block->IsEmpty());
            int processed = block->CollectHandles(context);
            assert(processed > 0 && processed < 64);
            total_processed += processed;
        }
        assert(total_processed == number_of_global_handles_);
    };
    
    // For all active, weak values that have not previously been marked for death but are currently
    // ready to die, save them from collection one last time so that the client has a chance to resurrect
    // them in callbacks before we make this final
    iso->performIncrementalMarking = [iso, this](JSCPrivate::JSMarkerRef marker, std::map<void*, JSObjectRef>& ready_to_die)
    {
        std::lock_guard<std::mutex> lock(iso->m_handlewalk_lock);

        for (NodeBlock *block = first_used_block_; block; block = block->next_used_block_) {
            block->PerformIncrementalMarking(marker, ready_to_die);
        }
        for (NodeBlock *block = first_block_; block; block = block->next_block_) {
            block->PerformIncrementalMarking(marker, ready_to_die);
        }
        
        for (auto i=ready_to_die.begin(); i!=ready_to_die.end(); ++i) {
            auto vi = ToImpl<V82JSC::Value>(&((Node*)i->first)->handle_);
            marker->Mark(marker, (JSObjectRef)vi->m_value);
        }
    };
    
    iso->weakObjectNearDeath = [this](internal::Object ** location,
                                   std::vector<v8::internal::SecondPassCallback>& callbacks,
                                   JSObjectRef object)
    {
        Node * node = reinterpret_cast<Node*>(location);
        int index = node->index_;
        intptr_t offset = reinterpret_cast<intptr_t>(&reinterpret_cast<NodeBlock*>(16)->handles_) - 16;
        intptr_t handle_array = reinterpret_cast<intptr_t>(location) - index * sizeof(Node);
        NodeBlock *block = reinterpret_cast<NodeBlock*>(handle_array - offset);

        assert((node->flags_ & kActiveWeakMask) == kActiveNearDeath ||
               (node->flags_ & kActiveWeakMask) == internal::Internals::kNodeStateIsWeakValue);
        SecondPassCallback second_pass;
        second_pass.callback_ = nullptr;
        second_pass.param_ = node->param_;
        second_pass.object_ = object;
        v8::Isolate *isolate = reinterpret_cast<v8::Isolate*>(block->global_handles_->isolate());
        if (object != 0) {
            v8::HandleScope handle_scope(isolate);
            JSContextRef ctx = ToContextRef(ToIsolateImpl(isolate)->m_nullContext.Get(isolate));
            auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, object);
            assert(wrap);
            second_pass.embedder_fields_[0] = wrap->m_embedder_data[0];
            second_pass.embedder_fields_[1] = wrap->m_embedder_data[1];
        }
        if (node->weak_callback_) {
            WeakCallbackInfo<void> info(isolate,
                                        node->param_,
                                        second_pass.embedder_fields_,
                                        &second_pass.callback_);
            node->weak_callback_(info);
        }
        if (second_pass.callback_) {
            callbacks.push_back(second_pass);
        }
    };

    iso->weakHeapObjectFinalized = [this](v8::Isolate *isolate, v8::internal::SecondPassCallback& callback)
    {
        assert(callback.callback_);
        WeakCallbackInfo<void> info(isolate,
                                    callback.param_,
                                    callback.embedder_fields_,
                                    nullptr);
        callback.callback_(info);
    };
    
    iso->weakJSObjectFinalized = [this](JSGlobalContextRef ctx, JSObjectRef obj)
    {
        IsolateImpl* iso = IsolateFromCtx(ctx);
        for (auto i=iso->m_second_pass_callbacks.begin(); i!=iso->m_second_pass_callbacks.end(); ++i) {
            if ((*i).object_ == obj) {
                (*i).ready_to_call = true;
            }
        }
    };
   
    if (!IsolateImpl::getIsolateFromGlobalHandle) {
        IsolateImpl::getIsolateFromGlobalHandle = [](v8::internal::Object **location) -> IsolateImpl* {
            auto handle_loc = reinterpret_cast<Node*>(location);
            int index = handle_loc->index_;
            intptr_t offset = reinterpret_cast<intptr_t>(&reinterpret_cast<internal::GlobalHandles::NodeBlock*>(16)->handles_) - 16;
            intptr_t handle_array = reinterpret_cast<intptr_t>(location) - index * sizeof(Node);
            NodeBlock *block = reinterpret_cast<NodeBlock*>(handle_array - offset);
            v8::Isolate *isolate = reinterpret_cast<v8::Isolate*>(block->global_handles_->isolate());
            IsolateImpl *iso = ToIsolateImpl(isolate);
            
            return iso;
        };
    }
}

internal::Handle<internal::Object> internal::GlobalHandles::Create(Object* value)
{
    if (!first_block_) {
        first_block_ = new NodeBlock(this);
    }
    return Handle<Object>(first_block_->New(value));
}

v8::internal::GetIsolateFromGlobalHandle IsolateImpl::getIsolateFromGlobalHandle;

void internal::GlobalHandles::Destroy(internal::Object** location)
{
    // Get NodeBlock from location
    Node * handle_loc = reinterpret_cast<Node*>(location);
    int index = handle_loc->index_;
    intptr_t offset = reinterpret_cast<intptr_t>(&reinterpret_cast<NodeBlock*>(16)->handles_) - 16;
    intptr_t handle_array = reinterpret_cast<intptr_t>(location) - index * sizeof(Node);
    NodeBlock *block = reinterpret_cast<NodeBlock*>(handle_array - offset);
    block->Reset(location);
}

internal::Handle<internal::Object> internal::GlobalHandles::CopyGlobal(v8::internal::Object **location)
{
    Node * handle_loc = reinterpret_cast<Node*>(location);
    int index = handle_loc->index_;
    intptr_t offset = reinterpret_cast<intptr_t>(&reinterpret_cast<NodeBlock*>(16)->handles_) - 16;
    intptr_t handle_array = reinterpret_cast<intptr_t>(location) - index * sizeof(Node);
    NodeBlock *block = reinterpret_cast<NodeBlock*>(handle_array - offset);
    
    Handle<Object> new_handle = block->global_handles_->Create(*location);
    // Copy traits (preserve index of new handle)
    internal::GlobalHandles::Node * new_handle_loc = reinterpret_cast<internal::GlobalHandles::Node*>(new_handle.location());
    
    new_handle_loc->classId_ = handle_loc->classId_;
    new_handle_loc->flags_   = handle_loc->flags_;
    new_handle_loc->embedder_fields_[0] = handle_loc->embedder_fields_[0];
    new_handle_loc->embedder_fields_[1] = handle_loc->embedder_fields_[1];
    new_handle_loc->weak_callback_ = handle_loc->weak_callback_;
    new_handle_loc->second_pass_callback_ = handle_loc->second_pass_callback_;
    new_handle_loc->param_   = handle_loc->param_;

    return new_handle;
}

void internal::GlobalHandles::MakeWeak(internal::Object **location, void *parameter,
                                       WeakCallbackInfo<void>::Callback weak_callback, v8::WeakCallbackType type)
{
    Node * handle_loc = reinterpret_cast<Node*>(location);
    int index = handle_loc->index_;
    intptr_t offset = reinterpret_cast<intptr_t>(&reinterpret_cast<NodeBlock*>(16)->handles_) - 16;
    intptr_t handle_array = reinterpret_cast<intptr_t>(location) - index * sizeof(Node);
    NodeBlock *block = reinterpret_cast<NodeBlock*>(handle_array - offset);
    v8::Isolate *isolate = reinterpret_cast<v8::Isolate*>(block->global_handles_->isolate());
    IsolateImpl *iso = ToIsolateImpl(isolate);
    
    v8::HandleScope scope(isolate);
    Local<v8::Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    if ((*location)->IsHeapObject()) {
        auto obj = FromHeapPointer(*location);
        BaseMap *map =
            reinterpret_cast<BaseMap*>(FromHeapPointer(obj->m_map));
        if (!(*location)->IsPrimitive() && (map == iso->m_value_map || map == iso->m_array_buffer_map)) {
            auto value = static_cast<V82JSC::Value*>(obj);
            if (JSValueIsObject(ctx, value->m_value)) {
                V82JSC::TrackedObject::makePrivateInstance(iso, ctx, (JSObjectRef)value->m_value);
                handle_loc->flags_ |= (1 << internal::Internals::kNodeIsActiveShift);
                WeakValue* weak =
                    static_cast<WeakValue*>
                    (HeapAllocator::Alloc(iso, iso->m_weak_value_map));
                weak->m_value = value->m_value;
                handle_loc->handle_ = ToHeapPointer(weak);
            }
        }
    }

    handle_loc->flags_ = (handle_loc->flags_ & ~internal::Internals::kNodeStateMask) |
        internal::Internals::kNodeStateIsWeakValue;
    handle_loc->param_ = parameter;
    handle_loc->type_ = type;
    handle_loc->weak_callback_ = weak_callback;
    handle_loc->second_pass_callback_ = nullptr;
}
void internal::GlobalHandles::MakeWeak(v8::internal::Object ***location_addr)
{
    MakeWeak(*location_addr, nullptr, nullptr, WeakCallbackType::kParameter);
}

void * internal::GlobalHandles::ClearWeakness(v8::internal::Object **location)
{
    Node * handle_loc = reinterpret_cast<Node*>(location);
    if ((handle_loc->flags_ & kActiveWeakMask) != kActiveWeak) return nullptr;

    int index = handle_loc->index_;
    intptr_t offset = reinterpret_cast<intptr_t>(&reinterpret_cast<NodeBlock*>(16)->handles_) - 16;
    intptr_t handle_array = reinterpret_cast<intptr_t>(location) - index * sizeof(Node);
    NodeBlock *block = reinterpret_cast<NodeBlock*>(handle_array - offset);
    v8::Isolate *isolate = reinterpret_cast<v8::Isolate*>(block->global_handles_->isolate());
    IsolateImpl *iso = ToIsolateImpl(isolate);

    v8::HandleScope scope(isolate);
    Local<v8::Context> context = OperatingContext(isolate);
    JSContextRef ctx = ToContextRef(context);
    if ((*location)->IsHeapObject()) {
        auto obj = FromHeapPointer(*location);
        if ((BaseMap*)FromHeapPointer(obj->m_map) == iso->m_weak_value_map) {
            WeakValue *value = static_cast<WeakValue*>(obj);
            auto strong = static_cast<V82JSC::Value*>(HeapAllocator::Alloc(iso, iso->m_value_map));
            strong->m_value = value->m_value;
            JSValueProtect(ctx, strong->m_value);
            handle_loc->handle_ = ToHeapPointer(strong);
        }
    }

    handle_loc->flags_ &= ~kActiveWeakMask;
    void *param = handle_loc->param_;
    handle_loc->param_ = nullptr;
    handle_loc->weak_callback_ = nullptr;
    handle_loc->second_pass_callback_ = nullptr;
    
    return param;
}

void internal::GlobalHandles::TearDown()
{
    for (NodeBlock *block = first_block_; block; ) {
        NodeBlock *next = block->next_block_;
        delete block;
        block = next;
    }
    for (NodeBlock *block = first_used_block_; block; ) {
        NodeBlock *next = block->next_used_block_;
        delete block;
        block = next;
    }
}

void V8::MakeWeak(internal::Object** location, void* data,
                  WeakCallbackInfo<void>::Callback weak_callback,
                  WeakCallbackType type)
{
    internal::GlobalHandles::MakeWeak(location, data, weak_callback, type);
}
void V8::MakeWeak(internal::Object** location, void* data,
                  // Must be 0 or -1.
                  int internal_field_index1,
                  // Must be 1 or -1.
                  int internal_field_index2,
                  WeakCallbackInfo<void>::Callback weak_callback)
{
    // FIXME:  Is this even being used?  If not, get rid of it.
    assert(0);
}
void V8::MakeWeak(internal::Object*** location_addr)
{
    internal::GlobalHandles::MakeWeak(location_addr);
}
void* V8::ClearWeak(internal::Object** location)
{
    return internal::GlobalHandles::ClearWeakness(location);
}

internal::Object** V8::GlobalizeReference(internal::Isolate* isolate,
                                          internal::Object** handle)
{
    return isolate->global_handles()->Create(*handle).location();
}
internal::Object** V8::CopyPersistent(internal::Object** handle)
{
    return internal::GlobalHandles::CopyGlobal(handle).location();
}
void V8::DisposeGlobal(internal::Object** global_handle)
{
    internal::GlobalHandles::Destroy(global_handle);
}
