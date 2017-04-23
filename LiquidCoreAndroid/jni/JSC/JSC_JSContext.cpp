//
// JSC_JSContext.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
#include "JSC.h"

OpaqueJSContext::OpaqueJSContext(JSContext *ctx) : m_context(ctx)
{
    ctx->Group()->RegisterGCCallback(StaticGCCallback, this);
}

OpaqueJSContext::~OpaqueJSContext()
{
    V8_ISOLATE(m_context->Group(), isolate);
        m_context->Group()->UnregisterGCCallback(StaticGCCallback, this);

        ForceGC();
        //For testing only.  Must also specify --enable_gc flag in common.cpp
        //isolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);

        m_gc_lock.lock();

        // First, look for all values that have a zero reference and clean them
        auto iterator = m_collection.begin();
        while (iterator != m_collection.end()) {
            const auto& v = *iterator;
            ++iterator;
            v->Clean(true);
        }

        // Then, release everything that has a reference count > 0
        bool isEmpty =  m_collection.empty();
        while (!isEmpty) {
            const auto& v = m_collection.front();
            const_cast<OpaqueJSValue *>(v)->Release();
            isEmpty =  m_collection.empty();
        }
        m_gc_lock.unlock();

        ASSERT(m_collection.empty());

        int count = m_context->release();
    V8_UNLOCK();
}

void OpaqueJSContext::MarkForCollection(JSValueRef value)
{
    ASSERT(value->Context() == this);
    m_gc_lock.lock();
    m_collection.push_back(value);
    m_gc_lock.unlock();
}

void OpaqueJSContext::MarkCollected(JSValueRef value)
{
    ASSERT(value->Context() == this);
    m_gc_lock.lock();
    m_collection.remove(value);
    m_gc_lock.unlock();
}

void OpaqueJSContext::GCCallback(GCType type, GCCallbackFlags flags)
{
}

void OpaqueJSContext::ForceGC()
{
    V8_ISOLATE(Context()->Group(), isolate)
        while(!isolate->IdleNotificationDeadline(
            group_->Platform()->MonotonicallyIncreasingTime() + 1.0)) {};
    V8_UNLOCK()
}

JS_EXPORT JSContextGroupRef JSContextGroupCreate()
{
    const ContextGroup *group = new ContextGroup();
    return group;
}

JS_EXPORT JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group)
{
    const_cast<ContextGroup*>(group)->retain();
    return group;
}

JS_EXPORT void JSContextGroupRelease(JSContextGroupRef group)
{
    ((ContextGroup*)group)->release();
}

static JSContextGroupRef globalContextGroup = nullptr;

class GlobalContextGroup : public ContextGroup
{
    public:
        GlobalContextGroup() : ContextGroup() {}
        virtual ~GlobalContextGroup() {
            globalContextGroup = nullptr;
        }
        virtual int release() {
            return --m_count;
            ASSERT(m_count >= 0);
        }
};

JS_EXPORT JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass)
{
    JSContextRef ctx = JSGlobalContextCreateInGroup(globalContextGroup, globalObjectClass);

    return (JSGlobalContextRef)ctx;
}

JS_EXPORT JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group,
    JSClassRef globalObjectClass)
{
    JSGlobalContextRef ctx;
    bool created = false;

    if (!group) {
        if (!globalContextGroup) {
            globalContextGroup = new GlobalContextGroup();
            created = true;
        }
        group = globalContextGroup;
    }

    {
        V8_ISOLATE((ContextGroup*)group,isolate)
            if (globalObjectClass) {
                Local<Object> data;
                Local<ObjectTemplate> templ;
                globalObjectClass->NewTemplate(group_->DefaultContext(), &data, &templ);
                Local<Context> context = Context::New(isolate, nullptr, templ);
                {
                    Context::Scope context_scope_(context);
                    Local<Object> global =
                        context->Global()->GetPrototype()->ToObject(context).ToLocalChecked();
                    ctx = new OpaqueJSContext(new JSContext((ContextGroup*)group, context));
                    TempJSValue value(globalObjectClass->InitInstance(ctx, global, data, nullptr));
                }
            } else {
                ctx = new OpaqueJSContext(
                    new JSContext((ContextGroup*)group, Context::New(isolate)));
            }

            // Apparently JavaScriptCore implements console.log out of the box.  V8 doesn't.
            Local<Context> context = ctx->Context()->Value();
            Context::Scope context_scope_(context);
            Local<Object> global =
                context->Global()->GetPrototype()->ToObject(context).ToLocalChecked();
            Local<Object> console = Object::New(isolate);
            Local<Object> Symbol =
                context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject();
            Local<Value> toStringTag = Symbol->Get(String::NewFromUtf8(isolate, "toStringTag"));
            Local<Object> consolePrototype = Object::New(isolate);
            consolePrototype->Set(context, toStringTag, String::NewFromUtf8(isolate, "Console"));
            console->SetPrototype(context, consolePrototype);
            global->DefineOwnProperty(context, String::NewFromUtf8(isolate, "console"), console,
                v8::DontEnum);
            Local<FunctionTemplate> logt = FunctionTemplate::New(isolate,
                [](const FunctionCallbackInfo< Value > &info) {
                    Isolate::Scope isolate_scope_(info.GetIsolate());
                    HandleScope handle_scope_(info.GetIsolate());

                    String::Utf8Value str(info[0]->ToString(info.GetIsolate()));
                    __android_log_print(ANDROID_LOG_INFO, "[JSC] console.log", "%s", *str);
                }
            );
            console->Set(context, String::NewFromUtf8(isolate, "log"),
                logt->GetFunction(context).ToLocalChecked());

        V8_UNLOCK()
    }

    if (created)
        ((ContextGroup*)group)->release();

    return ctx;
}

JS_EXPORT JSGlobalContextRef JSGlobalContextRetain(JSGlobalContextRef ctx)
{
    ctx->retain();
    return ctx;
}

JS_EXPORT void JSGlobalContextRelease(JSGlobalContextRef ctx)
{
    ctx->release();
}

JS_EXPORT JSObjectRef JSContextGetGlobalObject(JSContextRef ctx)
{
    JSObjectRef v;

    V8_ISOLATE_CTX(ctx->Context(),isolate,Ctx)
        JSValue<Object> *object = context_->Global();
        Local<Object> global = object->Value();
        v = OpaqueJSValue::New(ctx, global);
        object->release();
    V8_UNLOCK()

    return v;
}

JS_EXPORT JSContextGroupRef JSContextGetGroup(JSContextRef ctx)
{
    return ctx->Context()->Group();
}

JS_EXPORT JSGlobalContextRef JSContextGetGlobalContext(JSContextRef ctx)
{
    // FIXME: What is this supposed to do?
    return (JSGlobalContextRef)ctx;
}

JS_EXPORT JSStringRef JSGlobalContextCopyName(JSGlobalContextRef ctx)
{
    JSStringRef n = nullptr;
    V8_ISOLATE_CTX(ctx->Context(),isolate,context)
        Local<Value> name = context->GetEmbedderData(0);
        if (!name->IsUndefined()) {
            String::Utf8Value str(name->ToString(context).ToLocalChecked());
            n = JSStringCreateWithUTF8CString(*str);
        }
    V8_UNLOCK()
    return n;
}

JS_EXPORT void JSGlobalContextSetName(JSGlobalContextRef ctx, JSStringRef name)
{
    V8_ISOLATE(ctx->Context()->Group(), isolate)
        if (name) {
            ctx->Context()->Value()->SetEmbedderData(0, name->Value(isolate));
        } else {
            ctx->Context()->Value()->
                SetEmbedderData(0, Local<Value>::New(isolate,Undefined(isolate)));
        }
    V8_UNLOCK()
}
