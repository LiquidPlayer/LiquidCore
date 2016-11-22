//
// Created by Eric on 11/6/16.
//

#include "JSC.h"

JS_EXPORT JSContextGroupRef JSContextGroupCreate()
{
    const ContextGroup *group = new ContextGroup();
    return group;
}

JS_EXPORT JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group)
{
    ((ContextGroup*)group)->retain();
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
        virtual ~GlobalContextGroup() { globalContextGroup = nullptr; }
};

JS_EXPORT JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass)
{
    JSContextRef ctx = JSGlobalContextCreateInGroup(globalContextGroup, globalObjectClass);

    return (JSGlobalContextRef)ctx;
}

JS_EXPORT JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group,
    JSClassRef globalObjectClass)
{
    JSContext *ctx;
    bool created = false;

    if (!group) {
        if (!globalContextGroup) {
            globalContextGroup = new GlobalContextGroup();
            created = true;
        }
        group = globalContextGroup;
    }

    {
        V8_ISOLATE((ContextGroup*)group,isolate);
            if (globalObjectClass) {
                Local<Object> data;
                Local<ObjectTemplate> templ = globalObjectClass->NewTemplate(&data);
                Local<Context> context = Context::New(isolate, nullptr, templ);
                Local<Object> global = context->Global();
                ctx = new JSContext((ContextGroup*)group, context);
                JSValueRef value = globalObjectClass->InitInstance(ctx, global, data);
                ((JSValue<Value>*)value)->release();
            } else {
                ctx = new JSContext((ContextGroup*)group, Context::New(isolate));
            }
        V8_UNLOCK();
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
    JSValue<Value> *v;

    V8_ISOLATE_CTX((JSContext*)ctx,isolate,Ctx);
        JSValue<Object> *object = context_->Global();
        v = JSValue<Value>::New(context_, object->Value());
        object->release();
    V8_UNLOCK();

    return v;
}

JS_EXPORT JSContextGroupRef JSContextGetGroup(JSContextRef ctx)
{
    ((JSContext*)ctx)->Group()->retain();
    return ((JSContext*)ctx)->Group();
}

JS_EXPORT JSGlobalContextRef JSContextGetGlobalContext(JSContextRef ctx)
{
    // FIXME: What is this supposed to do?
    // FIXME: Gets the global context of a JavaScript execution context.
    ((JSContext*)ctx)->retain();
    return (JSGlobalContextRef)ctx;
}

JS_EXPORT JSStringRef JSGlobalContextCopyName(JSGlobalContextRef ctx)
{
    JSStringRef n;
    V8_ISOLATE_CTX((JSContext*)ctx,isolate,context);
        Local<Value> name = context->GetEmbedderData(1);
        String::Utf8Value str(name->ToString(context).ToLocalChecked());
        n = JSStringCreateWithUTF8CString(*str);
    V8_UNLOCK();
    return n;
}

JS_EXPORT void JSGlobalContextSetName(JSGlobalContextRef ctx, JSStringRef name)
{
    V8_ISOLATE_CTX((JSContext*)ctx,isolate,context);
        context->SetEmbedderData(1, name->Value(isolate));
    V8_UNLOCK();
}
