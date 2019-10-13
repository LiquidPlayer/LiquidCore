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

static std::map<ObjectData*,JSContextRef> s_ObectDataMap;
static std::recursive_mutex s_ObjectDataMapMutex;

Local<Value> ObjectData::New(const JSClassDefinition *def, JSContextRef ctx, JSClassRef cls)
{
    Isolate *isolate = Isolate::GetCurrent();
    EscapableHandleScope scope(isolate);

    auto od = new ObjectData(def, ctx, cls);
    Local<External> data = External::New(isolate, od);

    od->m_weak.Reset(isolate, data);
    od->m_weak.SetWeak<ObjectData>(
        od,
        [](const WeakCallbackInfo<ObjectData>& info) {

        delete info.GetParameter();
    }, v8::WeakCallbackType::kParameter);

    return scope.Escape(data);
}

ObjectData* ObjectData::Get(Local<Value> value)
{
    return (ObjectData*) value.As<External>()->Value();
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

    m_func.Reset(isolate, func);
    m_func.SetWeak();
}

ObjectData::~ObjectData()
{
    if (m_name) free(m_name);
    m_name = nullptr;

    m_func.Reset();
    m_weak.Reset();

    std::unique_lock<std::recursive_mutex> lk(s_ObjectDataMapMutex);
    s_ObectDataMap.erase(this);
}

ObjectData::ObjectData(const JSClassDefinition *def, JSContextRef ctx, JSClassRef cls) :
    m_definition(def), m_context(ctx), m_class(cls), m_name(nullptr)
{
    std::unique_lock<std::recursive_mutex> lk(s_ObjectDataMapMutex);
    s_ObectDataMap[this] = ctx;
}

void ObjectData::Clean(JSContextRef ctx)
{
    std::unique_lock<std::recursive_mutex> lk(s_ObjectDataMapMutex);
    for (auto it=s_ObectDataMap.begin(); it!=s_ObectDataMap.end(); ) {
        if (it->second == ctx) {
            auto od = it->first;
            s_ObectDataMap.erase(it++);
            delete od;
        } else {
            ++it;
        }
    }
}