/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

/* This file implements the V8_DEPRECATE_SOON functions that are
 * still in use by node.
 */

void Isolate::SetAutorunMicrotasks(bool autorun)
{
    SetMicrotasksPolicy(autorun ? MicrotasksPolicy::kAuto : MicrotasksPolicy::kExplicit);
}

Local<String> Message::GetSourceLine() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    return scope.Escape(GetSourceLine(isolate->GetCurrentContext()).ToLocalChecked());
}

int Message::GetLineNumber() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return GetLineNumber(isolate->GetCurrentContext()).ToChecked();
}

Local<Integer> Value::ToInteger(Isolate* isolate) const
{
    return ToInteger(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<Value> Function::Call(Local<Value> recv, int argc, Local<Value> *argv)
{
    Isolate* isolate = V82JSC::ToIsolate(this);

    return Call(isolate->GetCurrentContext(), recv, argc, argv).ToLocalChecked();
}

bool Value::Equals(Local<Value> other) const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Equals(isolate->GetCurrentContext(), other).FromJust();
}

int64_t Value::IntegerValue() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return IntegerValue(isolate->GetCurrentContext()).FromJust();
}

int32_t Value::Int32Value() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Int32Value(isolate->GetCurrentContext()).FromJust();
}

bool Value::BooleanValue() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return BooleanValue(isolate->GetCurrentContext()).FromJust();
}

uint32_t Value::Uint32Value() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Uint32Value(isolate->GetCurrentContext()).FromJust();
}

double Value::NumberValue() const
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return NumberValue(isolate->GetCurrentContext()).FromJust();
}

Local<Object> Value::ToObject(Isolate* isolate) const
{
    return ToObject(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<String> Value::ToString(Isolate* isolate) const
{
    return ToString(isolate->GetCurrentContext()).ToLocalChecked();
}

bool Object::Set(unsigned int index, Local<Value> value)
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Set(isolate->GetCurrentContext(), index, value).ToChecked();
}
bool Object::Set(Local<Value> key, Local<Value> value)
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Set(isolate->GetCurrentContext(), key, value).ToChecked();
}
Local<Value> Object::Get(Local<Value> key)
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Get(isolate->GetCurrentContext(), key).ToLocalChecked();
}
Isolate* Object::GetIsolate()
{
    return V82JSC::ToIsolate(this);
}
Local<Array> Object::GetPropertyNames()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return GetPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
}
Local<Value> Object::Get(unsigned int index)
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Get(isolate->GetCurrentContext(), index).ToLocalChecked();
}
Local<Array> Object::GetOwnPropertyNames()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return GetOwnPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
}

// Sets an own property on this object bypassing interceptors and
// overriding accessors or read-only properties.
//
// Note that if the object has an interceptor the property will be set
// locally, but since the interceptor takes precedence the local property
// will only be returned if the interceptor doesn't return a value.
//
// Note also that this only works for named properties.
Maybe<bool> Object::ForceSet(Local<Context> context,
                             Local<Value> key, Local<Value> value,
                             PropertyAttribute attribs)
{
    return DefineOwnProperty(context, key.As<Name>(), value, attribs);
}


Local<String> String::NewFromUtf8(Isolate* isolate, char const* str, String::NewStringType type, int length)
{
    return String::NewFromUtf8(isolate, str,
                               type==String::NewStringType::kNormalString ?
                               v8::NewStringType::kNormal : v8::NewStringType::kInternalized,
                               length).ToLocalChecked();
}

Local<Value> Script::Run()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return Run(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<Function> FunctionTemplate::GetFunction()
{
    Isolate* isolate = V82JSC::ToIsolate(this);
    return GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<Context> Debug::GetDebugContext(Isolate* isolate)
{
    return Local<Context>();
}

bool Debug::SetDebugEventListener (Isolate* isolate , void (*)(Debug::EventDetails const&), Local<Value>)
{
    return false;
}

void Debug::DebugBreak(v8::Isolate*)
{
}

CpuProfiler * Isolate::GetCpuProfiler()
{
    if (Isolate::GetCurrent()) {
        return CpuProfiler::New(Isolate::GetCurrent());
    }
    return NULL;
}

void HeapProfiler::StartTrackingHeapObjects(bool track)
{
    
}

void HeapProfiler::SetWrapperClassInfoProvider(uint16_t class_id,
                                               WrapperInfoCallback callback)
{
    
}
