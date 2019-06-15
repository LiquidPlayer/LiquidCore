/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <cstdlib>
#include "JSC/OpaqueJSClass.h"
#include "JSC/ObjectData.h"
#include "JSC/OpaqueJSContext.h"

Local<Value> ObjectData::New(const JSClassDefinition *def, JSContextRef ctx, JSClassRef cls)
{
    Isolate *isolate = Isolate::GetCurrent();
    EscapableHandleScope scope(isolate);

    ObjectData *od = new ObjectData(def, ctx, cls);
    char ptr[32];
    sprintf(ptr, "%p", od);
    Local<String> data =
        String::NewFromUtf8(isolate, ptr, NewStringType::kNormal).ToLocalChecked();

    UniquePersistent<Value> m_weak = UniquePersistent<Value>(isolate, data);
    m_weak.SetWeak<ObjectData>(
        od,
        [](const WeakCallbackInfo<ObjectData>& info) {

        delete info.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    return scope.Escape(data);
}

ObjectData* ObjectData::Get(Local<Value> value)
{
    String::Utf8Value const str(Isolate::GetCurrent(), value);
    unsigned long n = strtoul(*str, NULL, 16);
    return (ObjectData*) n;
}

void ObjectData::SetContext(JSContextRef ctx)
{
    m_context = ctx;
}

void ObjectData::SetName(Local<Value> name)
{
    String::Utf8Value const str(Isolate::GetCurrent(), name);
    m_name = (char *) malloc(strlen(*str) + 1);
    strcpy(m_name, *str);
}

void ObjectData::SetFunc(Local<Object> func)
{
    Isolate *isolate = Isolate::GetCurrent();

    m_func = Persistent<Object,CopyablePersistentTraits<Object>>(isolate, func);
}

ObjectData::~ObjectData()
{
    if (m_name) free(m_name);
    m_name = nullptr;

    m_func.Reset();
}

ObjectData::ObjectData(const JSClassDefinition *def, JSContextRef ctx, JSClassRef cls) :
    m_definition(def), m_context(ctx), m_class(cls), m_name(nullptr)
{
}
