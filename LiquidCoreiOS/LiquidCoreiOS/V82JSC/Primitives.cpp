//
//  Number.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

double Number::Value() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);

    return JSValueToNumber(ctx, value, 0);
}

Local<Number> Number::New(Isolate* isolate, double value)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    ValueImpl *num = static_cast<ValueImpl *>(HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate), sizeof(ValueImpl)));
    V82JSC::Map(num)->set_instance_type(v8::internal::HEAP_NUMBER_TYPE);
    num->m_value = JSValueMakeNumber(ctx, value);
    JSValueProtect(ctx, num->m_value);

    return V82JSC::CreateLocal<Number>(isolate, num);
}

Local<Integer> Integer::New(Isolate* isolate, int32_t value)
{
    return Number::New(isolate, value).As<Integer>();
}
Local<Integer> Integer::NewFromUnsigned(Isolate* isolate, uint32_t value)
{
    return Number::New(isolate, value).As<Integer>();
}
int64_t Integer::Value() const
{
    return reinterpret_cast<const Number*>(this)->Value();
}

int32_t Int32::Value() const
{
    return reinterpret_cast<const Number*>(this)->Value();
}

uint32_t Uint32::Value() const
{
    return reinterpret_cast<const Number*>(this)->Value();
}

bool v8::Boolean::Value() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);

    return JSValueToBoolean(ctx, value);
}

/**
 * Returns the identity hash for this object. The current implementation
 * uses an inline property on the object to store the identity hash.
 *
 * The return value will never be 0. Also, it is not guaranteed to be
 * unique.
 */
int Name::GetIdentityHash()
{
    return 1;
}

Local<Primitive> ValueImpl::NewUndefined(v8::Isolate *isolate)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    ValueImpl *undefined = static_cast<ValueImpl *>(HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate), sizeof(ValueImpl)));
    V82JSC::Map(undefined)->set_instance_type(v8::internal::ODDBALL_TYPE);
    internal::Oddball* oddball_handle = reinterpret_cast<internal::Oddball*>(V82JSC::Map(undefined));
    oddball_handle->set_kind(internal::Internals::kUndefinedOddballKind);
    undefined->m_value = JSValueMakeUndefined(ctx);
    JSValueProtect(ctx, undefined->m_value);

    return V82JSC::CreateLocal<Primitive>(isolate, undefined);
}

Local<Primitive>  ValueImpl::NewNull(v8::Isolate *isolate)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    ValueImpl *null = static_cast<ValueImpl *>(HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate), sizeof(ValueImpl)));
    V82JSC::Map(null)->set_instance_type(v8::internal::ODDBALL_TYPE);
    internal::Oddball* oddball_handle = reinterpret_cast<internal::Oddball*>(V82JSC::Map(null));
    oddball_handle->set_kind(internal::Internals::kNullOddballKind);
    null->m_value = JSValueMakeNull(ctx);
    JSValueProtect(ctx, null->m_value);

    return V82JSC::CreateLocal<Primitive>(isolate, null);
}

Local<Primitive>  ValueImpl::NewBoolean(v8::Isolate *isolate, bool value)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    ValueImpl *is = static_cast<ValueImpl *>(HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate), sizeof(ValueImpl)));
    is->map.set_instance_type(v8::internal::JS_VALUE_TYPE);
    is->m_value = JSValueMakeBoolean(ctx, value);
    JSValueProtect(ctx, is->m_value);

    return V82JSC::CreateLocal<Primitive>(isolate, is);
}

