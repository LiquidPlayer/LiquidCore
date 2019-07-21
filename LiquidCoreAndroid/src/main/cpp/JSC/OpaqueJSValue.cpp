/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
            Context::Scope context_scope(ctx->Context()->Value());
            Local<Object> o = v->ToObject(context).ToLocalChecked();
            o = o->StrictEquals(context->Global()) && \
                !o->GetPrototype()->ToObject(context).IsEmpty() && \
                o->GetPrototype()->ToObject(context).ToLocalChecked()->InternalFieldCount()>=
                    INSTANCE_OBJECT_FIELDS ?
                o->GetPrototype()->ToObject(context).ToLocalChecked() : \
                o;
            if (o->InternalFieldCount() >= INSTANCE_OBJECT_FIELDS) {
                out=
                    reinterpret_cast<OpaqueJSValue*>(
                        o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_JSOBJECT));
                auto clazz = reinterpret_cast<OpaqueJSClass*>(
                        o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_CLASS));
                if (out) {
                    out->Retain();
                } else if (clazz) {
                    clazz->retain();
                }
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
            if (o->InternalFieldCount() >= INSTANCE_OBJECT_FIELDS) {
                auto clazz = reinterpret_cast<OpaqueJSClass*>(
                        o->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_CLASS));
                if (!clazz)
                    __android_log_assert("clazz", "OpaqueJSValue", "clazz is NULL");
                if (clazz)
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

bool OpaqueJSValue::Clean() const
{
    if (m_count <= 0) {
        boost::shared_ptr<JSValue> value = m_value;
        if (value) {
            const_cast<OpaqueJSValue *>(this)->m_value = boost::shared_ptr<JSValue>();
            value.reset();
        }
        const JSClassDefinition *definition = m_fromClassDefinition;
        if (weak.IsEmpty()) {
            if (definition && !HasFinalized()) {
                const_cast<OpaqueJSValue *>(this)->m_finalized = true;
                while (definition) {
                    if (definition->finalize) {
                        definition->finalize(const_cast<JSObjectRef>(this));
                    }
                    definition = definition->parentClass ?
                            definition->parentClass->Definition() : nullptr;
                }
            }
            delete this;
        }
        return true;
    }
    return false;
}

int OpaqueJSValue::Retain()
{
    boost::shared_ptr<JSValue> value = m_value;
    if (!value) {
        V8_ISOLATE(m_ctx->Context()->Group(), isolate)
            Context::Scope context_scope(m_ctx->Context()->Value());
            m_value = JSValue::New(m_ctx->Context(), Local<Value>::New(isolate,weak));
        V8_UNLOCK()
    }
    return ++m_count;
}

int OpaqueJSValue::Release()
{
    int count = --m_count;
    ASSERTJSC(count >= 0)
    Clean();
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
    m_value(nullptr), m_ctx(context), m_fromClassDefinition(fromClass), m_count(0)
{
    weak.Reset(context->Context()->isolate(), v);
    weak.SetWeak<OpaqueJSValue>(this, [](const WeakCallbackInfo<OpaqueJSValue> &info) {
        (info.GetParameter())->WeakCallback();
    }, v8::WeakCallbackType::kParameter);
    const_cast<OpaqueJSContext *>(context)->MarkForCollection(this);
}

void OpaqueJSValue::WeakCallback() {
    weak.Reset();
    Clean();
}
