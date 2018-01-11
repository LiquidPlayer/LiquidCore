//
// OpaqueJSValue.h
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

#include "JSC/Macros.h"
#include "JavaScriptCore/JavaScript.h"
#include "JSC/OpaqueJSContext.h"
#include "JSC/OpaqueJSValue.h"
#include "JSC/OpaqueJSClass.h"

JSValueRef OpaqueJSValue::New(JSContextRef ctx, Local<Value> v,
    const JSClassDefinition* fromClass)
{
    OpaqueJSValue* out = nullptr;
    V8_ISOLATE(ctx->Context()->Group(), isolate)
        if (v->IsObject() && !fromClass) {
            Local<v8::Context> context = ctx->Context()->Value();
            Context::Scope(ctx->Context()->Value());
            Local<Object> o = v->ToObject(context).ToLocalChecked();
            o = o->StrictEquals(context->Global()) && \
                !o->GetPrototype()->ToObject(context).IsEmpty() && \
                o->GetPrototype()->ToObject(context).ToLocalChecked()->InternalFieldCount()>
                    INSTANCE_OBJECT_JSOBJECT ?
                o->GetPrototype()->ToObject(context).ToLocalChecked() : \
                o;
            if (o->InternalFieldCount() > INSTANCE_OBJECT_JSOBJECT) {
                out=
                    reinterpret_cast<OpaqueJSValue*>(
                        o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_JSOBJECT));
                out->Retain();
            }
        }
    V8_UNLOCK()
    if (!out) {
        out = new OpaqueJSValue(ctx, v, fromClass);
    }
    return out;
}

JSValueRef OpaqueJSValue::New(JSContextRef context, const char *s)
{
    ASSERTJSC(s);
    Local<String> local = String::NewFromUtf8(context->Context()->isolate(),s);
    return new OpaqueJSValue(context, local);
}

OpaqueJSValue::~OpaqueJSValue()
{
    V8_ISOLATE(m_ctx->Context()->Group(), isolate)
        Local<Value> v = L();
        if (*v && v->IsObject()) {
            Local<v8::Context> context = m_ctx->Context()->Value();
            Local<Object> o = v->ToObject(context).ToLocalChecked();
            if (o->InternalFieldCount() > INSTANCE_OBJECT_JSOBJECT) {
                OpaqueJSClass* clazz =
                    reinterpret_cast<OpaqueJSClass*>(o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_CLASS));
                clazz->release();
                o->SetAlignedPointerInInternalField(INSTANCE_OBJECT_CLASS, nullptr);
                o->SetAlignedPointerInInternalField(INSTANCE_OBJECT_JSOBJECT, nullptr);
                /* Note: A weak callback will only retain the first two internal fields
                 * But the first one is reserved.  So we will null out the second one.
                 * I am intentionally not using the macro here to ensure that we always
                 * zero out position one, even if the indices move later.
                 */
                o->SetAlignedPointerInInternalField(1, nullptr);
            }
        }
        const_cast<OpaqueJSContext *>(m_ctx)->MarkCollected(this);
        weak.Reset();
    V8_UNLOCK()
}

void OpaqueJSValue::Clean(bool fromGC) const
{
    if (m_count <= 0) {
        if (!HasFinalized()) {
            const_cast<OpaqueJSValue *>(this)->m_finalized = true;
            const JSClassDefinition * definition = m_fromClassDefinition;
            while (definition) {
                if (definition->finalize) {
                    definition->finalize(const_cast<JSObjectRef>(this));
                }
                definition =
                    definition->parentClass? definition->parentClass->Definition(): nullptr;
            }
        }
        delete this;
    }
}

int OpaqueJSValue::Retain()
{
    if (!value) {
        V8_ISOLATE(m_ctx->Context()->Group(), isolate)
            Context::Scope(m_ctx->Context()->Value());
            value = JSValue::New(m_ctx->Context(), Local<Value>::New(isolate,weak));
            weak.Reset();
        V8_UNLOCK()
    }
    return ++m_count;
}

int OpaqueJSValue::Release(bool cleanOnZero)
{
    int count = --m_count;
    ASSERTJSC(count >= 0)
    if (cleanOnZero) {
        Clean();
    }
    return count;
}

bool OpaqueJSValue::SetPrivateData(void *data)
{
    if (m_fromClassDefinition) {
        m_private_data = data;
    }
    return m_fromClassDefinition != nullptr;
}

OpaqueJSValue::OpaqueJSValue(JSContextRef context, Local<Value> v, const JSClassDefinition* fromClass) :
    value(nullptr), m_ctx(context), m_fromClassDefinition(fromClass), m_count(1)
{
    weak = UniquePersistent<Value>(context->Context()->isolate(), v);
    weak.SetWeak<OpaqueJSValue>(this, [](const WeakCallbackInfo<OpaqueJSValue> &info) {
        (info.GetParameter())->WeakCallback();
    }, v8::WeakCallbackType::kParameter);
    const_cast<OpaqueJSContext *>(context)->MarkForCollection(this);
}

void OpaqueJSValue::WeakCallback() {
    weak.Reset();
}
