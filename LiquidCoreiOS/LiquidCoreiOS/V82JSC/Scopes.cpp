//
//  Scopes.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

HandleScope::HandleScope(Isolate* isolate)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(isolate);
    
    this->isolate_ = reinterpret_cast<internal::Isolate*>(isolate);
    this->prev_next_ = reinterpret_cast<internal::Object **>(impl->m_handle_index);
    this->prev_limit_ = nullptr;
}

HandleScope::~HandleScope()
{
    // FIXME: Here is where we would decrement count the internal objects
}

int HandleScope::NumberOfHandles(Isolate* isolate)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(reinterpret_cast<Isolate*>(isolate));
    
    return impl->m_handle_index;
}

internal::Object** HandleScope::CreateHandle(internal::Isolate* isolate,
                                       internal::Object* value)
{
    IsolateImpl* impl = V82JSC::ToIsolateImpl(reinterpret_cast<Isolate*>(isolate));
    if (impl->m_handle_index % MAX_HANDLES_PER_GROUP == 0) {
        impl->m_handles.push_back(internal::HandleGroup());
    }
    internal::HandleGroup& group = impl->m_handles.back();
    internal::Object ** handle = & group.handles_[impl->m_handle_index % MAX_HANDLES_PER_GROUP];
    impl->m_handle_index++;
    *handle = value;
    return handle;
}

internal::Object** HandleScope::CreateHandle(internal::HeapObject* heap_object,
                                       internal::Object* value)
{
    assert(0);
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

