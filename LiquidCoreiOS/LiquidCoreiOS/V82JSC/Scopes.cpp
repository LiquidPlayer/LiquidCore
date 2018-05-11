//
//  Scopes.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

#define HANDLEBLOCK_SIZE 0x10000
struct HandleBlock {
    internal::Object * handles_[HANDLEBLOCK_SIZE / internal::kApiPointerSize];
};


HandleScope::HandleScope(Isolate* isolate)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(isolate);
    
    isolate_ = reinterpret_cast<internal::Isolate*>(isolate);
    prev_next_ = impl->i.ii.handle_scope_data()->next;
    prev_limit_ = impl->i.ii.handle_scope_data()->limit;

    impl->m_scope_stack.push(this);
}

HandleScope::~HandleScope()
{
    IsolateImpl* impl = reinterpret_cast<IsolateImpl*>(isolate_);
    
    impl->i.ii.handle_scope_data()->next = prev_next_;
    impl->i.ii.handle_scope_data()->limit = prev_limit_;
    
    impl->m_scope_stack.pop();
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
int internal::HandleScope::NumberOfHandles(Isolate* isolate)
{
    assert(0);
    return 0;
}

// Extend the handle scope making room for more handles.
internal::Object** internal::HandleScope::Extend(Isolate* isolate)
{
    IsolateImpl* isolateimpl = reinterpret_cast<IsolateImpl*>(isolate);
    DCHECK(!isolateimpl->m_scope_stack.empty());
    
    // FIXME: Deal with actual expanison later
    assert(isolateimpl->i.ii.handle_scope_data()->next == nullptr);

    HandleBlock *ptr;
    posix_memalign((void**)&ptr, HANDLEBLOCK_SIZE, sizeof(HandleBlock));

    internal::Object **handles = &ptr->handles_[0];
    HandleScopeData *data = isolateimpl->i.ii.handle_scope_data();
    data->next = handles;
    data->limit = &handles[HANDLEBLOCK_SIZE / internal::kApiPointerSize];
    return handles;
}

internal::Object** internal::CanonicalHandleScope::Lookup(Object* object)
{
    assert(0);
    return nullptr;
}

int HandleScope::NumberOfHandles(Isolate* isolate)
{
    //IsolateImpl* impl = V82JSC::ToIsolateImpl(reinterpret_cast<Isolate*>(isolate));
    
    printf("FIXME! HandleScope::NumberOfHandles\n");
    return 1;
}

internal::Object** HandleScope::CreateHandle(internal::Isolate* isolate,
                                       internal::Object* value)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(reinterpret_cast<Isolate*>(isolate));
    return internal::HandleScope::CreateHandle(&impl->i.ii, value);
}

internal::Object** HandleScope::CreateHandle(internal::HeapObject* heap_object,
                                       internal::Object* value)
{
    return CreateHandle(heap_object->GetIsolate(), value);
}

EscapableHandleScope::EscapableHandleScope(Isolate* isolate) : HandleScope(isolate)
{
    // Super HACK!
    this->escape_slot_ = reinterpret_cast<internal::Object**>(isolate);
}

internal::Object** EscapableHandleScope::Escape(internal::Object** escape_value)
{
    // Super HACK!
    return HandleScope::CreateHandle(reinterpret_cast<internal::Isolate*>(escape_slot_), *escape_value);
}

SealHandleScope::~SealHandleScope()
{
    
}

SealHandleScope::SealHandleScope(Isolate* isolate) : isolate_(nullptr)
{
    
}

MicrotasksScope::MicrotasksScope(Isolate* isolate, Type type) : isolate_(nullptr)
{
    
}
MicrotasksScope::~MicrotasksScope()
{
    
}

/**
 * Runs microtasks if no kRunMicrotasks scope is currently active.
 */
void MicrotasksScope::PerformCheckpoint(Isolate* isolate)
{
    
}

/**
 * Returns current depth of nested kRunMicrotasks scopes.
 */
int MicrotasksScope::GetCurrentDepth(Isolate* isolate)
{
    return 0;
}

/**
 * Returns true while microtasks are being executed.
 */
bool MicrotasksScope::IsRunningMicrotasks(Isolate* isolate)
{
    return false;
}

class internal::GlobalHandles::Node {
public:
    union {
        internal::Object* handle_;
        uint8_t reserved_[internal::kApiPointerSize * 2];
    };
    int index_;
};

class internal::GlobalHandles::NodeBlock {
public:
    NodeBlock(GlobalHandles *global_handles) {
        global_handles_ = global_handles;
        next_used_block_ = nullptr;
        prev_used_block_ = nullptr;
        bitmap_ = 0xffffffffffffffff;
        next_block_ = global_handles->first_block_;
        if (next_block_) next_block_->prev_block_ = this;
        prev_block_ = nullptr;
        global_handles->first_block_ = this;
    }
    internal::Object ** New(internal::Object *value)
    {
        if (bitmap_ == 0) return nullptr;
        int rightmost_set_bit_index = ffsll(bitmap_) - 1;
        uint64_t mask = pow(2,rightmost_set_bit_index);
        bitmap_ &= ~mask;
        if (bitmap_ == 0) {
            // Remove from available pool and add to used list
            if (prev_block_) {
                prev_block_->next_block_ = next_block_;
            } else {
                global_handles_->first_block_ = next_block_;
            }
            next_block_ = nullptr;
            prev_block_ = nullptr;
            next_used_block_ = global_handles_->first_used_block_;
            if (next_used_block_) next_used_block_->prev_used_block_ = this;
            prev_used_block_ = nullptr;
            global_handles_->first_used_block_ = this;
        }
        global_handles_->number_of_global_handles_ ++;
        memset(&handles_[rightmost_set_bit_index], 0, sizeof(Node));
        handles_[rightmost_set_bit_index].index_ = (int) rightmost_set_bit_index;
        handles_[rightmost_set_bit_index].handle_ = value;
        if(value->IsHeapObject()) {
            InternalObjectImpl *impl = reinterpret_cast<InternalObjectImpl*>(reinterpret_cast<intptr_t>(value) - internal::kHeapObjectTag);
            impl->Retain();
        }
        return &handles_[rightmost_set_bit_index].handle_;
    }
    void Reset(internal::Object ** handle)
    {
        internal::Object *value = *handle;
        int64_t index = (reinterpret_cast<intptr_t>(handle) - reinterpret_cast<intptr_t>(&handles_[0])) / sizeof(Node);
        assert(index >= 0 && index < 64);
        bool wasFull = bitmap_ == 0;
        uint64_t mask = pow(2,index);
        assert((bitmap_ & mask) == 0);
        bitmap_ |= mask;
        global_handles_->number_of_global_handles_ --;
        if (bitmap_ == 0xffffffffffffffff) {
            // NodeBlock is empty, remove from available list and delete
            if (prev_block_) {
                prev_block_->next_block_ = next_block_;
            } else {
                global_handles_->first_block_ = next_block_;
            }
            delete this;
        } else if (wasFull) {
            // Remove from used list and add to avaialable pool
            if (prev_used_block_) {
                prev_used_block_->next_used_block_ = next_used_block_;
            } else {
                global_handles_->first_used_block_ = next_used_block_;
            }
            prev_used_block_ = nullptr;
            next_used_block_ = nullptr;
            next_block_ = global_handles_->first_block_;
            global_handles_->first_block_ = this;
            prev_block_ = nullptr;
        }
        if (value->IsHeapObject()) {
            InternalObjectImpl *impl = reinterpret_cast<InternalObjectImpl*>(reinterpret_cast<intptr_t>(value) - internal::kHeapObjectTag);
            impl->Release();
        }
    }
    
private:
    NodeBlock *next_block_;
    NodeBlock *prev_block_;
    NodeBlock *next_used_block_;
    NodeBlock *prev_used_block_;
    GlobalHandles *global_handles_;
    uint64_t bitmap_;
    Node handles_[64];
    friend class internal::GlobalHandles;
};

internal::GlobalHandles::GlobalHandles(internal::Isolate *isolate)
{
    isolate_ = isolate;
    number_of_global_handles_ = 0;
    first_block_ = nullptr;
    first_used_block_ = nullptr;
    first_free_ = nullptr;
}

internal::Handle<internal::Object> internal::GlobalHandles::Create(Object* value)
{
    if (!first_block_) {
        new NodeBlock(this);
    }
    return Handle<Object>(first_block_->New(value));
}

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
    index = new_handle_loc->index_;
    memcpy(new_handle_loc, handle_loc, sizeof(Node));
    new_handle_loc->index_ = index;
    
    return new_handle;
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
