//
//  shim.cpp
//  LiquidCoreiOSTests
//
//  Created by Eric Lange on 2/10/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include <climits>
#include <csignal>
#include <map>
#include <memory>
#include <string>

#include "test/cctest/test-api.h"

#if V8_OS_POSIX
#include <unistd.h>  // NOLINT
#endif

#include "include/v8-util.h"
#include "src/api.h"
#include "src/arguments.h"
#include "src/base/platform/platform.h"
#include "src/code-stubs.h"
#include "src/compilation-cache.h"
#include "src/debug/debug.h"
#include "src/execution.h"
#include "src/futex-emulation.h"
#include "src/heap/incremental-marking.h"
#include "src/lookup.h"
#include "src/objects-inl.h"
#include "src/parsing/preparse-data.h"
#include "src/profiler/cpu-profiler.h"
#include "src/unicode-inl.h"
#include "src/utils.h"
#include "src/vm-state.h"
#include "test/cctest/heap/heap-tester.h"
#include "test/cctest/heap/heap-utils.h"
#include "test/cctest/profiler-extension.h"
#include "test/cctest/print-extension.h"
#include "test/cctest/trace-extension.h"

using namespace v8;

void CpuProfiler::Dispose() {}
void CpuProfiler::StartProfiling(Local<String> string, bool b) {}
CpuProfiler * CpuProfiler::New(Isolate* isolate) { return nullptr; }

void debug::SetDebugDelegate(Isolate* isolate, v8::debug::DebugDelegate* listener) {}

using namespace v8::internal;

//
// internal::Factory
//

// Creates a new on-heap JSTypedArray.
internal::Handle<JSTypedArray> Factory::NewJSTypedArray(ElementsKind elements_kind,
                                                        size_t number_of_elements,
                                                        PretenureFlag pretenure)
{
    return Handle<JSTypedArray>();
}
internal::Handle<internal::String> Factory::InternalizeOneByteString(Vector<const uint8_t> str)
{
    return Handle<internal::String>();
}

internal::Handle<FixedArray> Factory::NewFixedArray(int size, PretenureFlag pretenure)
{
    return Handle<FixedArray>();
}

MaybeHandle<internal::String> Factory::NewExternalStringFromOneByte(const ExternalOneByteString::Resource* resource)
{
    return MaybeHandle<internal::String>();
}

//
// internal::String
//
template <>
void internal::String::WriteToFlat(internal::String* source, unsigned short* sink, int from, int to)
{
    
}
internal::Handle<internal::String> internal::String::SlowFlatten(Handle<ConsString> cons,
                           PretenureFlag tenure)
{
    return Handle<String>();
}
bool internal::String::SlowEquals(internal::String* other)
{
    return false;
}

bool internal::String::SlowAsArrayIndex(uint32_t* index)
{
    return false;
}

bool internal::String::MakeExternal(v8::String::ExternalStringResource* resource)
{
    return false;
}

internal::Handle<internal::String> StringTable::LookupString(Isolate* isolate, internal::Handle<internal::String> string)
{
    return Handle<internal::String>();
}

//
// internal::Heap
//

// Returns of size of all objects residing in the heap.
size_t Heap::SizeOfObjects()
{
    return 0;
}

// Performs garbage collection operation.
// Returns whether there is a chance that another major GC could
// collect more garbage.
bool Heap::CollectGarbage(AllocationSpace space, GarbageCollectionReason gc_reason,
                          const GCCallbackFlags gc_callback_flags)
{
    return false;
}

// Performs a full garbage collection.  If (flags & kMakeHeapIterableMask) is
// non-zero, then the slower precise sweeper is used, which leaves the heap
// in a state where we can iterate over the heap visiting all objects.
void Heap::CollectAllGarbage(int flags, GarbageCollectionReason gc_reason,
                             const GCCallbackFlags gc_callback_flags)
{
    
}

// Last hope GC, should try to squeeze as much as possible.
void Heap::CollectAllAvailableGarbage(GarbageCollectionReason gc_reason)
{
    
}

bool Heap::ShouldOptimizeForMemoryUsage()
{
    return false;
}

// Start incremental marking and ensure that idle time handler can perform
// incremental steps.
void Heap::StartIdleIncrementalMarking(GarbageCollectionReason gc_reason,
                                       GCCallbackFlags gc_callback_flags)
{
    
}

//
// internal::HeapIterator
//
HeapIterator::HeapIterator(Heap* heap, HeapObjectsFiltering filtering)
{
    
}
HeapIterator::~HeapIterator()
{
    
}
HeapObject* HeapIterator::next()
{
    return nullptr;
}

//
// internal::heap
//
void heap::SimulateFullSpace(v8::internal::NewSpace* space,
                       std::vector<Handle<FixedArray>>* out_handles)
{
    
}

// Helper function that simulates a full old-space in the heap.
void heap::SimulateFullSpace(v8::internal::PagedSpace* space)
{
    
}

//
// internal::IncrementalMarking
//

void IncrementalMarking::RecordWriteSlow(HeapObject* obj, Object** slot, Object* value)
{
    *slot = nullptr;
}

//
// Allocation
//
void internal::FatalProcessOutOfMemory(const char* message)
{
    
}

char* internal::StrDup(const char* str)
{
    return strdup(str);
}

void* Malloced::New(size_t size)
{
    return nullptr;
}

void Malloced::Delete(void* p)
{
    
}

//
// internal::HandleScope
//
// Deallocates any extensions used by the current scope.
void internal::HandleScope::DeleteExtensions(Isolate* isolate)
{
    
}

// Counts the number of allocated handles.
int internal::HandleScope::NumberOfHandles(Isolate* isolate)
{
    return 0;
}

// Extend the handle scope making room for more handles.
internal::Object** internal::HandleScope::Extend(Isolate* isolate)
{
    return nullptr;
}

internal::Object** CanonicalHandleScope::Lookup(Object* object)
{
    return nullptr;
}

//
// internal::LookupIterator
//
template <bool is_element>
void LookupIterator::Start() {
}
template void LookupIterator::Start<false>();
template void LookupIterator::Start<true>();

internal::Handle<JSReceiver> LookupIterator::GetRootForNonJSReceiver(Isolate* isolate,
                                                           Handle<Object> receiver, uint32_t index)
{
    return Handle<JSReceiver>();
}
internal::Handle<internal::Object> LookupIterator::GetAccessors() const {
    return Handle<internal::Object>();
}

//
// internal::Isolate
//
bool internal::Isolate::IsFastArrayConstructorPrototypeChainIntact()
{
    return false;
}
MaybeHandle<JSPromise> internal::Isolate::RunHostImportModuleDynamicallyCallback(
                                                              Handle<String> referrer, Handle<Object> specifier)
{
    return MaybeHandle<JSPromise>();
}
base::RandomNumberGenerator* internal::Isolate::random_number_generator()
{
    return nullptr;
}
const intptr_t internal::Isolate::is_profiling_debug_offset_ = 0;
const intptr_t internal::Isolate::per_isolate_assert_data_debug_offset_ = 0;

//
// internal::CpuFeatures
//
void CpuFeatures::PrintTarget() {}
void CpuFeatures::PrintFeatures() {}
// Platform-dependent implementation.
void CpuFeatures::ProbeImpl(bool cross_compile) {}
bool CpuFeatures::initialized_ = false;

//
// Utils
//
void PRINTF_FORMAT(1, 2) internal::PrintF(const char* format, ...) {}
void PRINTF_FORMAT(2, 3) internal::PrintF(FILE* out, const char* format, ...) {}
// Safe formatting print. Ensures that str is always null-terminated.
// Returns the number of chars written, or -1 if output was truncated.
int PRINTF_FORMAT(2, 3) internal::SNPrintF(Vector<char> str, const char* format, ...) { return 0; }

//
// internal::CpuProfiler
//
void internal::CpuProfiler::DeleteAllProfiles() {}

//
// internal::Builtins
//
internal::Handle<Code> Builtins::InterpreterEnterBytecodeDispatch()
{
    return Handle<Code>();
}
internal::Handle<Code> Builtins::InterpreterEnterBytecodeAdvance()
{
    return Handle<Code>();
}
internal::Handle<Code> Builtins::InterpreterEntryTrampoline()
{
    return Handle<Code>();
}

//
// internal::Object
//
// Returns the permanent hash code associated with this object. May return
// undefined if not yet created.
internal::Object* internal::Object::GetHash() { return nullptr; }

// Returns the permanent hash code associated with this object depending on
// the actual object type. May create and store a hash code if needed and none
// exists.
Smi* internal::Object::GetOrCreateHash(Isolate* isolate, Handle<Object> object)
{
    return nullptr;
}

MaybeHandle<internal::Object> internal::Object::GetProperty(LookupIterator* it)
{
    return MaybeHandle<Object>();
}

//
// internal::MessageHandler
//
internal::Handle<JSMessageObject> MessageHandler::MakeMessageObject(
                                                 Isolate* isolate, MessageTemplate::Template type,
                                                 const MessageLocation* location, Handle<Object> argument,
                                                 Handle<FixedArray> stack_frames)
{
    return Handle<JSMessageObject>();
}
// Report a formatted message (needs JS allocation).
void MessageHandler::ReportMessage(Isolate* isolate, const MessageLocation* loc,
                          Handle<JSMessageObject> message)
{
}

//
// internal::AccountingAllocator
//
AccountingAllocator::AccountingAllocator() {}
AccountingAllocator::~AccountingAllocator() {}
// Gets an empty segment from the pool or creates a new one.
Segment* AccountingAllocator::GetSegment(size_t bytes)
{
    return nullptr;
}
// Return unneeded segments to either insert them into the pool or release
// them if the pool is already full or memory pressure is high.
void AccountingAllocator::ReturnSegment(Segment* memory)
{
}

//
// internal::Zone
//
Zone::Zone(AccountingAllocator* allocator, const char* name) {}
Zone::~Zone() {}

//
// internal::CompilationCache
//
void CompilationCache::Clear() {}

//
// internal::V8
//
Platform* internal::V8::GetCurrentPlatform() { return nullptr; }

//
// Extensions
//
Local<FunctionTemplate> ProfilerExtension::GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Local<v8::String> name)
{
    return Local<FunctionTemplate>();
}
const char* ProfilerExtension::kSource = "ProfilerExtension";
Local<FunctionTemplate> PrintExtension::GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Local<v8::String> name)
{
    return Local<FunctionTemplate>();
}
Local<FunctionTemplate> TraceExtension::GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Local<v8::String> name)
{
    return Local<FunctionTemplate>();
}
const char* TraceExtension::kSource = "TraceExtension";

//
// internal::TimedHistogram
//
// Start the timer. Log if isolate non-null.
void TimedHistogram::Start(base::ElapsedTimer* timer, Isolate* isolate) {}
void TimedHistogram::Stop(base::ElapsedTimer* timer, Isolate* isolate) {}

//
// internal::Script
//
// Init line_ends array with source code positions of line ends.
void internal::Script::InitLineEnds(Handle<Script> script) {}
bool internal::Script::GetPositionInfo(int position, PositionInfo* info,
                     OffsetFlag offset_flag) const
{
    return false;
}

//
// internal::ScriptData
//
ScriptData::ScriptData(const byte* data, int length) {}

//
// internal::FixedArrayBase
//
bool FixedArrayBase::IsCowArray() const { return false; }

//
// internal::JSReceiver
//
internal::Handle<internal::Context> JSReceiver::GetCreationContext()
{
    return Handle<Context>();
}

//
// internal::StackGuard
//
void StackGuard::RequestInterrupt(InterruptFlag flag) {}

//
// internal:MarkCompactCollector
//
void MarkCompactCollector::EnsureSweepingCompleted() {}

//
// internal::MessageLocation
//
MessageLocation::MessageLocation(Handle<Script> script, int start_pos, int end_pos) {}

std::ostream& internal::operator<<(std::ostream& os, v8::internal::InstanceType i)
{
    return os;
}
