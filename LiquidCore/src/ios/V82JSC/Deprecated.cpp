/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "Message.h"

using namespace V82JSC;
using v8::Isolate;
using v8::Local;
using v8::Integer;
using v8::Function;
using v8::Object;
using v8::Array;
using v8::Maybe;

/* This file implements the V8_DEPRECATE_SOON functions that are
 * still in use by node.
 */

void Isolate::SetAutorunMicrotasks(bool autorun)
{
    SetMicrotasksPolicy(autorun ? MicrotasksPolicy::kAuto : MicrotasksPolicy::kExplicit);
}

Local<v8::String> v8::Message::GetSourceLine() const
{
    Isolate* isolate = ToIsolate(this);
    EscapableHandleScope scope(isolate);
    
    return scope.Escape(GetSourceLine(isolate->GetCurrentContext()).ToLocalChecked());
}

int v8::Message::GetLineNumber() const
{
    Isolate* isolate = ToIsolate(this);
    return GetLineNumber(isolate->GetCurrentContext()).ToChecked();
}

Local<Integer> v8::Value::ToInteger(Isolate* isolate) const
{
    return ToInteger(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<v8::Value> Function::Call(Local<Value> recv, int argc, Local<Value> *argv)
{
    Isolate* isolate = ToIsolate(this);

    return Call(isolate->GetCurrentContext(), recv, argc, argv).ToLocalChecked();
}

bool v8::Value::Equals(Local<Value> other) const
{
    Isolate* isolate = ToIsolate(this);
    return Equals(isolate->GetCurrentContext(), other).FromJust();
}

int64_t v8::Value::IntegerValue() const
{
    Isolate* isolate = ToIsolate(this);
    return IntegerValue(isolate->GetCurrentContext()).FromJust();
}

int32_t v8::Value::Int32Value() const
{
    Isolate* isolate = ToIsolate(this);
    return Int32Value(isolate->GetCurrentContext()).FromJust();
}

bool v8::Value::BooleanValue() const
{
    Isolate* isolate = ToIsolate(this);
    return BooleanValue(isolate->GetCurrentContext()).FromJust();
}

uint32_t v8::Value::Uint32Value() const
{
    Isolate* isolate = ToIsolate(this);
    return Uint32Value(isolate->GetCurrentContext()).FromJust();
}

double v8::Value::NumberValue() const
{
    Isolate* isolate = ToIsolate(this);
    return NumberValue(isolate->GetCurrentContext()).FromJust();
}

Local<Object> v8::Value::ToObject(Isolate* isolate) const
{
    return ToObject(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<v8::String> v8::Value::ToString(Isolate* isolate) const
{
    return ToString(isolate->GetCurrentContext()).ToLocalChecked();
}

bool Object::Set(unsigned int index, Local<Value> value)
{
    Isolate* isolate = ToIsolate(this);
    return Set(isolate->GetCurrentContext(), index, value).ToChecked();
}
bool Object::Set(Local<Value> key, Local<Value> value)
{
    Isolate* isolate = ToIsolate(this);
    return Set(isolate->GetCurrentContext(), key, value).ToChecked();
}
Local<v8::Value> Object::Get(Local<Value> key)
{
    Isolate* isolate = ToIsolate(this);
    return Get(isolate->GetCurrentContext(), key).ToLocalChecked();
}
Isolate* Object::GetIsolate()
{
    return ToIsolate(this);
}
Local<Array> Object::GetPropertyNames()
{
    Isolate* isolate = ToIsolate(this);
    return GetPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
}
Local<v8::Value> Object::Get(unsigned int index)
{
    Isolate* isolate = ToIsolate(this);
    return Get(isolate->GetCurrentContext(), index).ToLocalChecked();
}
Local<Array> Object::GetOwnPropertyNames()
{
    Isolate* isolate = ToIsolate(this);
    return GetOwnPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<v8::String> v8::String::NewFromUtf8(Isolate* isolate, char const* str, String::NewStringType type, int length)
{
    return String::NewFromUtf8(isolate, str,
                               type==String::NewStringType::kNormalString ?
                               v8::NewStringType::kNormal : v8::NewStringType::kInternalized,
                               length).ToLocalChecked();
}

int v8::String::WriteUtf8(
    char* buffer,
    int length,
    int* nchars_ref,
    int options) const
{
    return v8::String::WriteUtf8(Isolate::GetCurrent(), buffer, length, nchars_ref, options);
}


Local<v8::Value> v8::Script::Run()
{
    Isolate* isolate = ToIsolate(this);
    return Run(isolate->GetCurrentContext()).ToLocalChecked();
}

Local<Function> v8::FunctionTemplate::GetFunction()
{
    Isolate* isolate = ToIsolate(this);
    return GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
}

v8::CpuProfiler * Isolate::GetCpuProfiler()
{
    if (Isolate::GetCurrent()) {
        return CpuProfiler::New(Isolate::GetCurrent());
    }
    return NULL;
}

void v8::HeapProfiler::StartTrackingHeapObjects(bool track)
{
    
}

void v8::HeapProfiler::SetWrapperClassInfoProvider(uint16_t class_id,
                                               WrapperInfoCallback callback)
{
    
}

v8::String::Utf8Value::Utf8Value(Local<v8::Value> obj) :
    v8::String::Utf8Value::Utf8Value(Isolate::GetCurrent(), obj)
{
}

Local<v8::StackFrame> v8::StackTrace::GetFrame(uint32_t index) const
{
    auto impl = ToImpl<V82JSC::StackTrace>(this);
    return GetFrame(ToIsolate(impl->GetIsolate()), index);
}

Local<v8::String> v8::String::Concat(Local<String> left, Local<String> right)
{
    return v8::String::Concat(Isolate::GetCurrent(), left, right);
}
