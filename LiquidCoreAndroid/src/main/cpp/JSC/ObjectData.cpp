//
// ObjectData.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    String::Utf8Value const str(value);
    unsigned long n = strtoul(*str, NULL, 16);
    return (ObjectData*) n;
}

void ObjectData::SetContext(JSContextRef ctx)
{
    m_context = ctx;
}

void ObjectData::SetName(Local<Value> name)
{
    String::Utf8Value const str(name);
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
