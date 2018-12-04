/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <boost/make_shared.hpp>
#include "JSC/JSC.h"

class GlobalContextGroup : public OpaqueJSContextGroup
{
    public:
        static boost::shared_ptr<GlobalContextGroup> New() {
            auto group = boost::make_shared<GlobalContextGroup>();
            group->m_self = group->shared_from_this();
            return group;
        }
        GlobalContextGroup() : OpaqueJSContextGroup() {}
};

static boost::shared_ptr<GlobalContextGroup> globalContextGroup;

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
    const_cast<OpaqueJSContextGroup*>(group)->Retain();

    auto cg = const_cast<OpaqueJSContextGroup*>(group)->ContextGroup::shared_from_this();
    V8_ISOLATE(cg, isolate)
        if (globalObjectClass) {
            ctx = globalObjectClass->NewContext(group);
        } else {
            ctx = &* OpaqueJSContext::New(JSContext::New(cg, Context::New(isolate)));
        }
        setUpJSCFeatures(ctx);
        const_cast<OpaqueJSContextGroup*>(group)->Release();
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
