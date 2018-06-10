//
//  shim.cpp
//  LiquidCoreiOSTests
//
//  Created by Eric Lange on 2/10/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

#include "test/cctest/heap/heap-tester.h"
#include "test/cctest/heap/heap-utils.h"
#include "test/cctest/profiler-extension.h"
#include "test/cctest/print-extension.h"
#include "test/cctest/trace-extension.h"

#ifdef __cplusplus
extern "C" {
#endif
    void JSSynchronousGarbageCollectForDebugging(JSContextRef);
#ifdef __cplusplus
}
#endif

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
    assert(0);
    return Handle<JSTypedArray>();
}
internal::Handle<internal::String> Factory::InternalizeOneByteString(Vector<const uint8_t> str)
{
    assert(0);
    return Handle<internal::String>();
}

internal::Handle<FixedArray> Factory::NewFixedArray(int size, PretenureFlag pretenure)
{
    assert(0);
    return Handle<FixedArray>();
}

MaybeHandle<internal::String> Factory::NewExternalStringFromOneByte(const ExternalOneByteString::Resource* resource)
{
    assert(0);
    return MaybeHandle<internal::String>();
}

//
// internal::String
//
template <>
void internal::String::WriteToFlat(internal::String* source, unsigned short* sink, int from, int to)
{
    assert(0);
}
internal::Handle<internal::String> internal::String::SlowFlatten(Handle<ConsString> cons,
                           PretenureFlag tenure)
{
    assert(0);
    return Handle<String>();
}
bool internal::String::SlowEquals(internal::String* other)
{
    assert(0);
    return false;
}

bool internal::String::SlowAsArrayIndex(uint32_t* index)
{
    assert(0);
    return false;
}

bool internal::String::MakeExternal(v8::String::ExternalStringResource* resource)
{
    assert(0);
    return false;
}

internal::Handle<internal::String> StringTable::LookupString(Isolate* isolate, internal::Handle<internal::String> string)
{
    ValueImpl *impl = reinterpret_cast<ValueImpl *>(V82JSC_HeapObject::FromHeapPointer(*string));
    impl->m_map = V82JSC_HeapObject::ToV8Map(reinterpret_cast<IsolateImpl*>(isolate)->m_internalized_string_map);
    return string;
}

//
// internal::Heap
//

// Returns of size of all objects residing in the heap.
size_t Heap::SizeOfObjects()
{
    assert(0);
    return 0;
}

// Performs garbage collection operation.
// Returns whether there is a chance that another major GC could
// collect more garbage.
bool Heap::CollectGarbage(AllocationSpace space, GarbageCollectionReason gc_reason,
                          const GCCallbackFlags gc_callback_flags)
{
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(isolate());
    // First pass, clear anything on the V82JSC side that is not in use
    iso->CollectGarbage();
    
    // Next, trigger garbage collection in JSC (do it twice -- sometimes the first doesn't finish the job)
    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
        JSSynchronousGarbageCollectForDebugging(i->first);
    }

    // Second pass, clear V82JSC garbage again in case any weak references were cleared
    iso->CollectGarbage();
    return false;
}

// Performs a full garbage collection.  If (flags & kMakeHeapIterableMask) is
// non-zero, then the slower precise sweeper is used, which leaves the heap
// in a state where we can iterate over the heap visiting all objects.
void Heap::CollectAllGarbage(int flags, GarbageCollectionReason gc_reason,
                             const GCCallbackFlags gc_callback_flags)
{
    IsolateImpl *iso = reinterpret_cast<IsolateImpl*>(isolate());
    // First pass, clear anything on the V82JSC side that is not in use
    iso->CollectGarbage();
    
    // Next, trigger garbage collection in JSC (do it twice -- sometimes the first doesn't finish the job)
    for (auto i=iso->m_global_contexts.begin(); i != iso->m_global_contexts.end(); ++i) {
        JSSynchronousGarbageCollectForDebugging(i->first);
        JSSynchronousGarbageCollectForDebugging(i->first);
    }
    
    // Second pass, clear V82JSC garbage again in case any weak references were cleared
    iso->CollectGarbage();
}

// Last hope GC, should try to squeeze as much as possible.
void Heap::CollectAllAvailableGarbage(GarbageCollectionReason gc_reason)
{
    reinterpret_cast<IsolateImpl*>(isolate())->CollectGarbage();
}

bool Heap::ShouldOptimizeForMemoryUsage()
{
    assert(0);
    return false;
}

// Start incremental marking and ensure that idle time handler can perform
// incremental steps.
void Heap::StartIdleIncrementalMarking(GarbageCollectionReason gc_reason,
                                       GCCallbackFlags gc_callback_flags)
{
    assert(0);
}

//
// internal::HeapIterator
//
HeapIterator::HeapIterator(Heap* heap, HeapObjectsFiltering filtering)
{
    assert(0);
}
HeapIterator::~HeapIterator()
{
    assert(0);
}
HeapObject* HeapIterator::next()
{
    assert(0);
    return nullptr;
}

//
// internal::heap
//
void heap::SimulateFullSpace(v8::internal::NewSpace* space,
                       std::vector<Handle<FixedArray>>* out_handles)
{
    assert(0);
}

// Helper function that simulates a full old-space in the heap.
void heap::SimulateFullSpace(v8::internal::PagedSpace* space)
{
    printf( "FIXME! heap::SimulateFullSpace\n" );
}

//
// internal::IncrementalMarking
//

void IncrementalMarking::RecordWriteSlow(HeapObject* obj, Object** slot, Object* value)
{
    assert(0);
    *slot = nullptr;
}

//
// Allocation
//
void internal::FatalProcessOutOfMemory(const char* message)
{
    assert(0);
}

char* internal::StrDup(const char* str)
{
    return strdup(str);
}

//
// internal::LookupIterator
//
template <bool is_element>
void LookupIterator::Start() {
    //assert(0);
}
template void LookupIterator::Start<false>();
template void LookupIterator::Start<true>();

internal::Handle<JSReceiver> LookupIterator::GetRootForNonJSReceiver(Isolate* isolate,
                                                           Handle<Object> receiver, uint32_t index)
{
    assert(0);
    return Handle<JSReceiver>();
}
internal::Handle<internal::Object> LookupIterator::GetAccessors() const {
    assert(0);
    return Handle<internal::Object>();
}

//
// internal::Isolate
//
bool internal::Isolate::IsFastArrayConstructorPrototypeChainIntact()
{
    assert(0);
    return false;
}
MaybeHandle<JSPromise> internal::Isolate::RunHostImportModuleDynamicallyCallback(
                                                              Handle<String> referrer, Handle<Object> specifier)
{
    assert(0);
    return MaybeHandle<JSPromise>();
}
base::RandomNumberGenerator* internal::Isolate::random_number_generator()
{
    assert(0);
    return nullptr;
}

//
// internal::CpuFeatures
//
void CpuFeatures::PrintTarget() {assert(0);}
void CpuFeatures::PrintFeatures() {assert(0);}
// Platform-dependent implementation.
void CpuFeatures::ProbeImpl(bool cross_compile) {assert(0);}
bool CpuFeatures::initialized_ = false;

//
// Utils
//
void PRINTF_FORMAT(1, 2) internal::PrintF(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
void PRINTF_FORMAT(2, 3) internal::PrintF(FILE* out, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(out, format, args);
    va_end(args);
}
// Safe formatting print. Ensures that str is always null-terminated.
// Returns the number of chars written, or -1 if output was truncated.
int PRINTF_FORMAT(2, 3) internal::SNPrintF(Vector<char> str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int r = vsnprintf(str.start(), str.size(), format, args);
    va_end(args);
    return r;
}

//
// internal::CpuProfiler
//
void internal::CpuProfiler::DeleteAllProfiles()
{
    printf ("FIXME! internal::CpuProfiler::DeleteAllProfiles()\n");
}

//
// internal::Builtins
//
internal::Handle<Code> Builtins::InterpreterEnterBytecodeDispatch()
{
    assert(0);
    return Handle<Code>();
}
internal::Handle<Code> Builtins::InterpreterEnterBytecodeAdvance()
{
    assert(0);
    return Handle<Code>();
}
internal::Handle<Code> Builtins::InterpreterEntryTrampoline()
{
    assert(0);
    return Handle<Code>();
}

//
// internal::Object
//
// Returns the permanent hash code associated with this object. May return
// undefined if not yet created.
internal::Object* internal::Object::GetHash()
{
    if (this->IsSmi()) return this;

    ValueImpl* impl = (ValueImpl*)(reinterpret_cast<uintptr_t>(this) & ~3);
    Local<v8::Context> context = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(impl))->GetCurrentContext();
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = impl->m_value;
    
    if (JSValueIsObject(ctx, value)) {
        TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef) value);
        if (!wrap) wrap = makePrivateInstance(V82JSC::ToIsolateImpl(impl), ctx, (JSObjectRef) value);
        return Smi::FromInt(wrap->m_hash);
    }
    return Smi::FromInt(JSValueToNumber(ctx, value, 0));
}

// Returns the permanent hash code associated with this object depending on
// the actual object type. May create and store a hash code if needed and none
// exists.
Smi* internal::Object::GetOrCreateHash(Isolate* isolate, Handle<Object> object)
{
    if ((*object.location())->IsSmi()) return (reinterpret_cast<Smi*>(*object.location()));
    
    HandleScope scope(isolate);
    
    Local<v8::Object> o = V82JSC::__local<v8::Object>(object.location()).toLocal();
    Local<v8::Context> context = V82JSC::ToCurrentContext(*o);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = V82JSC::ToJSValueRef(o, context);
    if (JSValueIsObject(ctx, value)) {
        int hash = o->GetIdentityHash();
        return Smi::FromInt(hash);
    }
    return nullptr;
}

MaybeHandle<internal::Object> internal::Object::GetProperty(LookupIterator* it)
{
    internal::Object *o = *it->GetReceiver();
    Local<v8::Object> obj = V82JSC::__local<v8::Object>(&o).toLocal();
    Local<v8::Context> context = V82JSC::ToCurrentContext(*obj);
    MaybeLocal<v8::Value> val = obj->Get(context, it->index());
    if (val.IsEmpty()) {
        return MaybeHandle<Object>();
    }
    return Handle<Object>(HandleScope::GetHandle(reinterpret_cast<internal::Isolate*>(V82JSC::ToIsolate(*obj)),
                                                 * reinterpret_cast<internal::Object**>(*val.ToLocalChecked())));
}

//
// internal::MessageHandler
//
internal::Handle<JSMessageObject> MessageHandler::MakeMessageObject(
                                                 Isolate* isolate, MessageTemplate::Template type,
                                                 const MessageLocation* location, Handle<Object> argument,
                                                 Handle<FixedArray> stack_frames)
{
    assert(0);
    return Handle<JSMessageObject>();
}
// Report a formatted message (needs JS allocation).
void MessageHandler::ReportMessage(Isolate* isolate, const MessageLocation* loc,
                          Handle<JSMessageObject> message)
{
    assert(0);
}

//
// internal::AccountingAllocator
//
AccountingAllocator::AccountingAllocator() {}
AccountingAllocator::~AccountingAllocator() {}
// Gets an empty segment from the pool or creates a new one.
Segment* AccountingAllocator::GetSegment(size_t bytes)
{
    assert(0);
    return nullptr;
}
// Return unneeded segments to either insert them into the pool or release
// them if the pool is already full or memory pressure is high.
void AccountingAllocator::ReturnSegment(Segment* memory)
{
    assert(0);
}

//
// internal::Zone
//
Zone::Zone(AccountingAllocator* allocator, const char* name) {assert(0);}
Zone::~Zone() {assert(0);}

//
// internal::CompilationCache
//
void CompilationCache::Clear()
{
    printf("FIXME! CompliationCache::Clear()\n");
}

//
// internal::V8
//
extern Platform* currentPlatform;
Platform* internal::V8::GetCurrentPlatform() { return currentPlatform; }

//
// Extensions
//
Local<FunctionTemplate> TraceExtension::GetNativeFunctionTemplate(v8::Isolate* isolate, v8::Local<v8::String> name)
{
    assert(0);
    return Local<FunctionTemplate>();
}
const char* TraceExtension::kSource = "";
//
// internal::TimedHistogram
//
// Start the timer. Log if isolate non-null.
void TimedHistogram::Start(base::ElapsedTimer* timer, Isolate* isolate) {assert(0);}
void TimedHistogram::Stop(base::ElapsedTimer* timer, Isolate* isolate) {assert(0);}

//
// internal::Script
//
// Init line_ends array with source code positions of line ends.
void internal::Script::InitLineEnds(Handle<Script> script) { assert(0); }
bool internal::Script::GetPositionInfo(int position, PositionInfo* info,
                     OffsetFlag offset_flag) const
{
    assert(0);
    return false;
}

//
// internal::ScriptData
//
ScriptData::ScriptData(const byte* data, int length)
{
    this->data_ = data;
    this->length_ = length;
}

//
// internal::FixedArrayBase
//
bool FixedArrayBase::IsCowArray() const { assert(0); return false; }

//
// internal::JSReceiver
//
internal::Handle<internal::Context> JSReceiver::GetCreationContext()
{
    assert(IsHeapObject());
    ValueImpl *impl = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(this) - internal::kHeapObjectTag);
    IsolateImpl* i = V82JSC::ToIsolateImpl(impl);
    v8::HandleScope scope(V82JSC::ToIsolate(i));
    Local<v8::Context> context = V82JSC::OperatingContext(V82JSC::ToIsolate(i));
    JSGlobalContextRef ctx = JSContextGetGlobalContext(V82JSC::ToContextRef(context));
    /*
    InstanceWrap *wrap = V82JSC::getPrivateInstance(ctx, (JSObjectRef)impl->m_value);
    if (wrap) ctx = wrap->m_creation_context;
    else ctx = JSObjectGetGlobalContext((JSObjectRef)impl->m_value);
    */
    assert(i->m_global_contexts.count(ctx) == 1);
    context = i->m_global_contexts[ctx].Get(V82JSC::ToIsolate(i));
    Handle<Context> c = * reinterpret_cast<Handle<Context>*>(&context);
    return c;
}

//
// internal::StackGuard
//
void StackGuard::RequestInterrupt(InterruptFlag flag) { assert(0); }

//
// internal:MarkCompactCollector
//
void MarkCompactCollector::EnsureSweepingCompleted() { assert(0); }

//
// internal::MessageLocation
//
MessageLocation::MessageLocation(Handle<Script> script, int start_pos, int end_pos) { assert(0); }

std::ostream& internal::operator<<(std::ostream& os, v8::internal::InstanceType i)
{
    assert(0);
    return os;
}
