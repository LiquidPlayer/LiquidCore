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
#include "JSC/JSC.h"

JS_EXPORT JSContextGroupRef JSContextGroupCreate()
{
    return &* OpaqueJSContextGroup::New();
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

class GlobalContextGroup : public OpaqueJSContextGroup
{
    public:
        static std::shared_ptr<GlobalContextGroup> New() {
            auto group = std::make_shared<GlobalContextGroup>();
            group->m_self = std::static_pointer_cast<ContextGroup>(
                const_cast<GlobalContextGroup*>(&*group)->shared_from_this()
            );
            return group;
        }
        GlobalContextGroup() : OpaqueJSContextGroup() {}
        /*
        virtual ~GlobalContextGroup() {
            globalContextGroup.reset();
        }
        */
};

static std::shared_ptr<GlobalContextGroup> globalContextGroup;

JS_EXPORT JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass)
{
    JSContextRef ctx = JSGlobalContextCreateInGroup(&* globalContextGroup, globalObjectClass);

    return (JSGlobalContextRef)ctx;
}

/*
 * Some features come out of the box in JavaScriptCore that are not there in V8.  We
 * simulate them here:
 *  - console.log()
 *  - global
 */
static void setUpJSCFeatures(JSGlobalContextRef ctx) {
    V8_ISOLATE_CTX(ctx->Context(), isolate, context)
        // Apparently JavaScriptCore implements console.log out of the box.  V8 doesn't.
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

        // global
        global->DefineOwnProperty(context, String::NewFromUtf8(isolate, "global"), context->Global(),
            v8::DontEnum);

    V8_UNLOCK()
}

JS_EXPORT JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group,
    JSClassRef globalObjectClass)
{
    JSGlobalContextRef ctx;

    if (!group) {
        if (!globalContextGroup) {
            globalContextGroup = GlobalContextGroup::New();
        }
        group = &* globalContextGroup;
    }
    const_cast<OpaqueJSContextGroup*>(group)->retain();

    auto cg = const_cast<OpaqueJSContextGroup*>(group)->ContextGroup::shared_from_this();
    V8_ISOLATE(cg, isolate)
        if (globalObjectClass) {
            ctx = globalObjectClass->NewContext(group);
        } else {
            ctx = &* OpaqueJSContext::New(JSContext::New(cg, Context::New(isolate)));
        }
        setUpJSCFeatures(ctx);
        const_cast<OpaqueJSContextGroup*>(group)->release();
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
    ctx->release();
}

JS_EXPORT JSObjectRef JSContextGetGlobalObject(JSContextRef ctx)
{
    JSObjectRef v;

    V8_ISOLATE_CTX(ctx->Context(),isolate,Ctx)
        Local<Value> global = ctx->Context()->Global()->Value();
        v = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, global));
    V8_UNLOCK()

    return v;
}

JS_EXPORT JSContextGroupRef JSContextGetGroup(JSContextRef ctx)
{
    return static_cast<JSContextGroupRef>(&* ctx->Context()->Group());
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
