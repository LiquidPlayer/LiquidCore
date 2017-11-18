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

OpaqueJSContextGroup::OpaqueJSContextGroup(Isolate *isolate, uv_loop_t *event_loop) :
    ContextGroup(isolate, event_loop), m_jsc_count(1)
{

}

OpaqueJSContextGroup::OpaqueJSContextGroup() : ContextGroup(), m_jsc_count(1)
{

}

OpaqueJSContextGroup::~OpaqueJSContextGroup()
{
}

void OpaqueJSContextGroup::AssociateContext(const OpaqueJSContext *ctx)
{
    m_mutex.lock();
    m_associatedContexts.push_back(ctx);
    m_mutex.unlock();
}

void OpaqueJSContextGroup::DisassociateContext(const OpaqueJSContext *ctx)
{
    m_mutex.lock();
    m_associatedContexts.remove(ctx);
    m_mutex.unlock();
}

void OpaqueJSContextGroup::Retain()
{
    m_jsc_count ++;
}

void OpaqueJSContextGroup::Release()
{
    if (--m_jsc_count == 0) {
        m_mutex.lock();
        while (!m_associatedContexts.empty()) {
            JSGlobalContextRef ctx = const_cast<JSGlobalContextRef>(m_associatedContexts.front());
            m_mutex.unlock();
            JSGlobalContextRelease(ctx);
            m_mutex.lock();
        }
        m_mutex.unlock();
        release();
    }
}

OpaqueJSContext::OpaqueJSContext(JSContext *ctx) : m_context(ctx)
{
    ctx->Group()->RegisterGCCallback(StaticGCCallback, this);
    static_cast<OpaqueJSContextGroup *>(ctx->Group())->AssociateContext(this);
}

OpaqueJSContext::~OpaqueJSContext()
{
    static_cast<OpaqueJSContextGroup *>(m_context->Group())->DisassociateContext(this);

    V8_ISOLATE(m_context->Group(), isolate);
        __android_log_print(ANDROID_LOG_DEBUG, "~OpaqueJSContext", "0");
        m_context->Group()->UnregisterGCCallback(StaticGCCallback, this);

        ForceGC();
        //For testing only.  Must also specify --enable_gc flag in common.cpp
        //isolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);

        m_gc_lock.lock();

        __android_log_print(ANDROID_LOG_DEBUG, "~OpaqueJSContext", "1");
        // First, look for all values that have a zero reference and clean them
        auto iterator = m_collection.begin();
        while (iterator != m_collection.end()) {
            const auto& v = *iterator;
            ++iterator;
            v->Clean(true);
        }
        __android_log_print(ANDROID_LOG_DEBUG, "~OpaqueJSContext", "2");

        // Then, release everything that has a reference count > 0
        bool isEmpty =  m_collection.empty();
        while (!isEmpty) {
            const auto& v = m_collection.front();
            const_cast<OpaqueJSValue *>(v)->Release();
            isEmpty =  m_collection.empty();
        }
        m_gc_lock.unlock();
        __android_log_print(ANDROID_LOG_DEBUG, "~OpaqueJSContext", "3");

        ASSERTJSC(m_collection.empty());

        int count = m_context->release();
        __android_log_print(ANDROID_LOG_DEBUG, "~OpaqueJSContext", "4");
    V8_UNLOCK();
}

void OpaqueJSContext::MarkForCollection(JSValueRef value)
{
    ASSERTJSC(value->Context() == this);
    m_gc_lock.lock();
    m_collection.push_back(value);
    m_gc_lock.unlock();
}

void OpaqueJSContext::MarkCollected(JSValueRef value)
{
    ASSERTJSC(value->Context() == this);
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
    JSContextGroupRef group = new OpaqueJSContextGroup();
    return group;
}

JS_EXPORT JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group)
{
    const_cast<OpaqueJSContextGroup*>(group)->Retain();
    return group;
}

JS_EXPORT void JSContextGroupRelease(JSContextGroupRef group)
{
    const_cast<OpaqueJSContextGroup*>(group)->Release();
}

static JSContextGroupRef globalContextGroup = nullptr;

class GlobalContextGroup : public OpaqueJSContextGroup
{
    public:
        GlobalContextGroup() : OpaqueJSContextGroup() {}
        virtual ~GlobalContextGroup() {
            globalContextGroup = nullptr;
        }
        virtual int release() {
            return --m_count;
            ASSERTJSC(m_count >= 0);
        }
};

JS_EXPORT JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass)
{
    JSContextRef ctx = JSGlobalContextCreateInGroup(globalContextGroup, globalObjectClass);

    return (JSGlobalContextRef)ctx;
}

static void setUpConsoleLog(JSGlobalContextRef ctx) {
    // Apparently JavaScriptCore implements console.log out of the box.  V8 doesn't.
    V8_ISOLATE_CTX(ctx->Context(), isolate, context)
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

JS_EXPORT JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group,
    JSClassRef globalObjectClass)
{
    JSGlobalContextRef ctx;

    if (!group) {
        if (!globalContextGroup) {
            globalContextGroup = new GlobalContextGroup();
        }
        group = globalContextGroup;
    }
    const_cast<OpaqueJSContextGroup*>(group)->retain();

    V8_ISOLATE((ContextGroup*)group, isolate)
        if (globalObjectClass) {
            ctx = globalObjectClass->NewContext(group);
        } else {
            ctx = new OpaqueJSContext(
                new JSContext((ContextGroup*)group, Context::New(isolate)));
        }
        setUpConsoleLog(ctx);
        ((ContextGroup*)group)->release();
    V8_UNLOCK()

    return ctx;
}

JS_EXPORT JSGlobalContextRef JSGlobalContextRetain(JSGlobalContextRef ctx)
{
    ctx->retain();
    return ctx;
}

JS_EXPORT void JSGlobalContextRelease(JSGlobalContextRef ctx)
{
    int count = ctx->release();
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
    return static_cast<JSContextGroupRef>(ctx->Context()->Group());
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
