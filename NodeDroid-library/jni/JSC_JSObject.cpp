//
// Created by Eric on 11/6/16.
//

#include "JSC.h"

#define V8_ISOLATE_OBJ(ctx,object,isolate,context,o) \
    V8_ISOLATE_CTX(ctx,isolate,context); \
    Local<Object> o = \
        reinterpret_cast<JSValue<Value>*>(object)->Value()->ToObject(context).ToLocalChecked();

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> value = (reinterpret_cast<JSValue<Value>*>(valueRef))->Value()

#define V8_ISOLATE_CALLBACK(info,isolate,context,definition) \
    Isolate::Scope isolate_scope_(info.GetIsolate()); \
    HandleScope handle_scope_(info.GetIsolate()); \
    Local<Object> obj_ = info.Data()->ToObject(); \
    const JSClassDefinition *definition = \
        (JSClassDefinition*) obj_->GetAlignedPointerFromInternalField(0);\
    V8_ISOLATE_CTX(obj_->GetAlignedPointerFromInternalField(1),isolate,context) \

#define VALUE(value) ((JSValue<Value> *)(value))
#define CTX(ctx)     ((JSContext *)(ctx))

OpaqueJSClass::OpaqueJSClass(const JSClassDefinition *definition) : m_definition(definition)
{
    if(m_definition->parentClass) {
        m_definition->parentClass->retain();
    }
}

OpaqueJSClass::~OpaqueJSClass()
{
    if (m_definition->parentClass) {
        m_definition->parentClass->release();
    }
}

void OpaqueJSClass::StaticAccessorGetter(Local< String > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);

        JSValueRef exception = nullptr;
        JSValueRef value = nullptr;

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        while (definition && !exception && !value) {
            for (int i=0;
                !value &&
                !exception &&
                definition->staticValues &&
                definition->staticValues[i].name &&
                strcmp(definition->staticValues[i].name, *str);
                i++) {

                value = definition->staticValues[i].getProperty(
                    context_,
                    thisObject,
                    string,
                    &exception);
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        string->release();
        thisObject->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }

        if (value) {
            info.GetReturnValue().Set(VALUE(value)->Value());
            VALUE(value)->release();
        }

    V8_UNLOCK();
}

void OpaqueJSClass::StaticAccessorSetter(Local< String > property, Local< Value > value,
    const PropertyCallbackInfo< void > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        JSValueRef exception = nullptr;

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        JSValueRef valueRef = JSValue<Value>::New(context_,value);
        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        bool set = false;
        while (definition && !exception && !set) {
            for (int i=0;
                !set &&
                !exception &&
                definition->staticValues &&
                definition->staticValues[i].name &&
                strcmp(definition->staticValues[i].name, *str);
                i++) {

                set = definition->staticValues[i].setProperty(
                    context_,
                    thisObject,
                    string,
                    valueRef,
                    &exception);
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        string->release();
        VALUE(valueRef)->release();
        thisObject->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }
    V8_UNLOCK();
}

void OpaqueJSClass::StaticFunctionAccessorGetter(Local< String > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
        templ->SetInternalFieldCount(2);
        Local<Object> data = templ->NewInstance();
        data->SetAlignedPointerInInternalField(0,(void*)definition);
        data->SetAlignedPointerInInternalField(1,(void*)context_);
        data->Set(context, String::NewFromUtf8(isolate, "name"), property);
        context_->retain();
        UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, data);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            [](const WeakCallbackInfo<UniquePersistent<Object>>& info) {

            JSContext* ctx = reinterpret_cast<JSContext*>(info.GetInternalField(1));
            ctx->release();
            info.GetParameter()->Reset();
            delete info.GetParameter();
        }, v8::WeakCallbackType::kInternalFields);

        Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
            StaticFunctionCallHandler, data);
        Local<Function> func = ftempl->GetFunction();

        data->Set(context, String::NewFromUtf8(isolate, "func"), func);

        info.GetReturnValue().Set(func);
    V8_UNLOCK();
}

void OpaqueJSClass::StaticFunctionCallHandler(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        String::Utf8Value const
            str(obj_->Get(context, String::NewFromUtf8(isolate, "name")).ToLocalChecked());

        JSValueRef arguments[info.Length()];
        for (int i=0; i<info.Length(); i++) {
            arguments[i] = JSValue<Value>::New(context_, info[i]);
        }
        JSObjectRef function = JSValue<Value>::New(context_, obj_->Get(context,
            String::NewFromUtf8(isolate, "func")).ToLocalChecked());
        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        JSValueRef exception = nullptr;
        JSValueRef value = nullptr;

        while (definition && !exception && !value) {
            for (int i=0;
                !value &&
                !exception &&
                definition->staticFunctions &&
                definition->staticFunctions[i].name &&
                strcmp(definition->staticFunctions[i].name, *str);
                i++) {

                value = definition->staticFunctions[i].callAsFunction(
                    context_,
                    function,
                    thisObject,
                    (size_t) info.Length(),
                    arguments,
                    &exception);
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        for (int i=0; i<info.Length(); i++) {
            VALUE(arguments[i])->release();
        }
        function->release();
        thisObject->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }

        if (value) {
            info.GetReturnValue().Set(VALUE(value)->Value());
            VALUE(value)->release();
        }

    V8_UNLOCK();
}

void OpaqueJSClass::NamedPropertyGetter(Local< String > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        JSValueRef exception = nullptr;

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        JSValueRef value = nullptr;

        while (definition && !exception && !value) {
            value = definition->getProperty(
                context_,
                thisObject,
                string,
                &exception);
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        thisObject->release();
        string->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }
        if (value) {
            info.GetReturnValue().Set(VALUE(value)->Value());
            VALUE(value)->release();
        }
    V8_UNLOCK();
}

void OpaqueJSClass::NamedPropertySetter(Local< String > property, Local< Value > value,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        JSValueRef exception = nullptr;

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());
        JSValueRef valueRef = JSValue<Value>::New(context_,value);

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        bool set = false;
        while (definition && !exception && !set) {
            set = definition->setProperty(
                context_,
                thisObject,
                string,
                valueRef,
                &exception);
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        thisObject->release();
        string->release();
        VALUE(valueRef)->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }
        if (set) {
            info.GetReturnValue().Set(value);
        }
    V8_UNLOCK();
}

void OpaqueJSClass::NamedPropertyDeleter(Local< String > property,
    const PropertyCallbackInfo< Boolean > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        JSValueRef exception = nullptr;

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        bool deleted = false;
        while (definition && !exception && !deleted) {
            deleted = definition->deleteProperty(
                context_,
                thisObject,
                string,
                &exception);
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        thisObject->release();
        string->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }
        if (deleted) {
            info.GetReturnValue().Set(deleted);
        }
    V8_UNLOCK();
}

void OpaqueJSClass::NamedPropertyEnumerator(const PropertyCallbackInfo< Array > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        OpaqueJSPropertyNameAccumulator accumulator;
        definition->getPropertyNames(
            context_,
            thisObject,
            &accumulator);

        Local<Array> array = Array::New(isolate, accumulator.size());
        for(size_t i=0; accumulator.size() > 0; i++) {
            array->Set(context, i, accumulator.back()->Value());
            accumulator.back()->release();
            accumulator.pop_back();
        }

        info.GetReturnValue().Set(array);

        thisObject->release();
    V8_UNLOCK();
}

void OpaqueJSClass::CallAsFunction(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition);
        JSValueRef exception = nullptr;

        JSValueRef arguments[info.Length()];
        for (int i=0; i<info.Length(); i++) {
            arguments[i] = JSValue<Value>::New(context_, info[i]);
        }
        JSObjectRef function = JSValue<Value>::New(context_, obj_->Get(context,
            String::NewFromUtf8(isolate, "func")).ToLocalChecked());
        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        JSValueRef value = nullptr;
        while (definition && !exception && !value) {
            if (info.IsConstructCall() && definition->callAsConstructor) {
                // FIXME: This may not work because 'value' may not get set as thisObject
                value = definition->callAsConstructor(
                    context_,
                    function,
                    (size_t) info.Length(),
                    arguments,
                    &exception);
            } else if (!info.IsConstructCall() && definition->callAsFunction) {
                value = definition->callAsFunction(
                    context_,
                    function,
                    thisObject,
                    (size_t) info.Length(),
                    arguments,
                    &exception);
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        for (int i=0; i<info.Length(); i++) {
            VALUE(arguments[i])->release();
        }
        function->release();
        thisObject->release();

        if (exception) {
            isolate->ThrowException(VALUE(exception)->Value());
            VALUE(exception)->release();
        }

        if (value) {
            info.GetReturnValue().Set(VALUE(value)->Value());
            VALUE(value)->release();
        }
    V8_UNLOCK();
}

void OpaqueJSClass::Finalize(const WeakCallbackInfo<UniquePersistent<Object>>& info)
{
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());

    OpaqueJSClass* clazz = reinterpret_cast<OpaqueJSClass*>(info.GetInternalField(0));
    JSObjectRef objRef = reinterpret_cast<JSObjectRef>(info.GetInternalField(1));

    const JSClassDefinition *definition = clazz->m_definition;
    while (definition) {
        if (definition->finalize) {
            definition->finalize(objRef);
        }
        definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
    }

    clazz->release();
    info.GetParameter()->Reset();
    delete info.GetParameter();
}

JSObjectRef OpaqueJSClass::NewInstance(JSContextRef ctx)
{
    JSValue<Value> *retObj = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        Local<ObjectTemplate> object = ObjectTemplate::New(isolate);

        Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
        templ->SetInternalFieldCount(2);
        Local<Object> data = templ->NewInstance();
        data->SetAlignedPointerInInternalField(0,(void*)m_definition);
        data->SetAlignedPointerInInternalField(1,(void*)ctx);
        CTX(ctx)->retain();
        UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, data);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            [](const WeakCallbackInfo<UniquePersistent<Object>>& info) {

            JSContext* ctx = reinterpret_cast<JSContext*>(info.GetInternalField(1));
            ctx->release();
            info.GetParameter()->Reset();
            delete info.GetParameter();
        }, v8::WeakCallbackType::kInternalFields);

        // Set up static values
        for (int i=0; m_definition->staticValues && m_definition->staticValues[i].name; i++) {
            int attributes = None;
            if (m_definition->staticValues[i].attributes | kJSPropertyAttributeReadOnly)
                attributes |= ReadOnly;
            if (m_definition->staticValues[i].attributes | kJSPropertyAttributeDontDelete)
                attributes |= DontDelete;
            if (m_definition->staticValues[i].attributes | kJSPropertyAttributeDontEnum)
                attributes |= DontEnum;
            object->SetAccessor(
                String::NewFromUtf8(isolate, m_definition->staticValues[i].name),
                StaticAccessorGetter,
                StaticAccessorSetter,
                data,
                DEFAULT,
                (PropertyAttribute) attributes
            );
        }

        // Set up static functions
        for (int i=0; m_definition->staticFunctions && m_definition->staticFunctions[i].name; i++) {
            Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
                StaticFunctionCallHandler, data);
            Local<Function> func = ftempl->GetFunction();

            data->Set(context, String::NewFromUtf8(isolate, "__static_func"), func);

            int attributes = None;
            if (m_definition->staticValues[i].attributes | kJSPropertyAttributeReadOnly)
                attributes |= ReadOnly;
            if (m_definition->staticValues[i].attributes | kJSPropertyAttributeDontDelete)
                attributes |= DontDelete;
            if (m_definition->staticValues[i].attributes | kJSPropertyAttributeDontEnum)
                attributes |= DontEnum;
            object->SetAccessor(
                String::NewFromUtf8(isolate, m_definition->staticValues[i].name),
                StaticFunctionAccessorGetter,
                nullptr,
                data,
                DEFAULT,
                (PropertyAttribute) attributes
            );
        }

        object->SetNamedPropertyHandler(
            m_definition->getProperty ? NamedPropertyGetter : nullptr,
            m_definition->setProperty ? NamedPropertySetter : nullptr,
            nullptr, // FIXME: hasProperty not implemented
            m_definition->deleteProperty ? NamedPropertyDeleter : nullptr,
            m_definition->getPropertyNames ? NamedPropertyEnumerator  : nullptr,
            data);

        if (m_definition->callAsFunction || m_definition->callAsConstructor) {
            object->SetCallAsFunctionHandler(CallAsFunction, data);
        }

        // FIXME: hasInstance not implemented
        // FIXME: convertToType not implemented

        object->SetInternalFieldCount(1);

        Local<Object> newObj = object->NewInstance();
        if (m_definition->callAsFunction || m_definition->callAsConstructor) {
            data->Set(context, String::NewFromUtf8(isolate, "func"), newObj);
        }
        retObj = JSValue<Value>::New(context_, newObj);

        weak = new UniquePersistent<Object>(isolate, newObj);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            Finalize,
            v8::WeakCallbackType::kInternalFields);

        newObj->SetAlignedPointerInInternalField(0,this);
        retain();

        // Find the greatest ancestor
        const JSClassDefinition *definition = nullptr;
        for (definition = m_definition; definition && definition->parentClass;
            definition = definition->parentClass->m_definition);

        // Walk backwards and call 'initialize on each'
        while (true) {
            definition->initialize(ctx, retObj);
            const JSClassDefinition *parent = definition;
            if (parent == m_definition) break;

            for (definition = m_definition;
                definition->parentClass && definition->parentClass->m_definition != parent;
                definition = definition->parentClass->m_definition);
        }
    V8_UNLOCK();

    return retObj;
}

JS_EXPORT JSClassRef JSClassCreate(const JSClassDefinition* definition)
{
    return new OpaqueJSClass(definition);
}

JS_EXPORT JSClassRef JSClassRetain(JSClassRef jsClass)
{
    jsClass->retain();
    return jsClass;
}

JS_EXPORT void JSClassRelease(JSClassRef jsClass)
{
    jsClass->release();
}

JS_EXPORT JSObjectRef JSObjectMake(JSContextRef ctx, JSClassRef jsClass, void* data)
{
    JSObjectRef value;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        value = jsClass->NewInstance(ctx);
        JSObjectSetPrivate(value, data);
    V8_UNLOCK();

    return value;
}

static JSObjectRef SetUpFunction(JSContextRef ctx, JSStringRef name, JSClassDefinition *definition)
{
    JSObjectRef obj;
    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
        templ->SetInternalFieldCount(2);
        Local<Object> data = templ->NewInstance();
        data->SetAlignedPointerInInternalField(0,definition);
        data->SetAlignedPointerInInternalField(1,(void*)context_);
        context_->retain();
        UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, data);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            [](const WeakCallbackInfo<UniquePersistent<Object>>& info) {

            JSContext* ctx = reinterpret_cast<JSContext*>(info.GetInternalField(1));
            JSClassDefinition *definition =
                reinterpret_cast<JSClassDefinition*>(info.GetInternalField(0));
            delete definition;
            ctx->release();
            info.GetParameter()->Reset();
            delete info.GetParameter();
        }, v8::WeakCallbackType::kInternalFields);

        Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
            OpaqueJSClass::CallAsFunction, data);
        Local<Function> func = ftempl->GetFunction();

        if (name) {
            func->SetName(name->Value());
        }

        data->Set(context, String::NewFromUtf8(isolate, "func"), func);

        obj = JSValue<Value>::New(context_, func);
    V8_UNLOCK();

    return obj;
}

JS_EXPORT JSObjectRef JSObjectMakeFunctionWithCallback(JSContextRef ctx, JSStringRef name,
    JSObjectCallAsFunctionCallback callAsFunction)
{
    JSClassDefinition *definition = new JSClassDefinition;
    memset(definition, 0, sizeof(JSClassDefinition));
    definition->callAsFunction = callAsFunction;

    return SetUpFunction(ctx, name, definition);
}

JS_EXPORT JSObjectRef JSObjectMakeConstructor(JSContextRef ctx, JSClassRef jsClass,
    JSObjectCallAsConstructorCallback callAsConstructor)
{
    JSClassDefinition *definition = new JSClassDefinition;
    memset(definition, 0, sizeof(JSClassDefinition));
    definition->callAsConstructor = callAsConstructor;

    JSObjectRef function = SetUpFunction(ctx, nullptr, definition);
    if (jsClass) {
        JSObjectRef proto = JSObjectMake(ctx, jsClass, nullptr);
        JSObjectSetPrototype(ctx, function, proto);
        proto->release();
    }

    return function;
}

JS_EXPORT JSObjectRef JSObjectMakeArray(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef object;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        Local<Array> array = Array::New(isolate, argumentCount);
        for(size_t i=0; i<argumentCount; i++) {
            array->Set(context, i, VALUE(arguments[i])->Value());
        }
        object = JSValue<Value>::New(context_, array);
    V8_UNLOCK();

    return object;
}

JS_EXPORT JSObjectRef JSObjectMakeDate(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef out;
    *exception = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        Local<Value> date;
        if (argumentCount==0) {
            Local<Object> DATE =
                context->Global()->Get(String::NewFromUtf8(isolate, "Date"))->ToObject();
            Local<Function> now = DATE->Get(String::NewFromUtf8(isolate, "now")).As<Function>();

            date = Date::New(isolate,
                now->Call(Local<Value>::New(isolate,Null(isolate)), 0, nullptr)
                    ->ToNumber(context).ToLocalChecked()->Value());
        } else {
            TryCatch trycatch(isolate);

            MaybeLocal<Number> number = VALUE(arguments[0])->Value()->ToNumber(context);
            double epoch = 0.0;
            if (!number.IsEmpty()) {
                epoch = number.ToLocalChecked()->Value();
            } else {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
                epoch = 0.0;
            }

            date = Date::New(isolate, epoch);
        }

        out = JSValue<Value>::New(context_, date);
    V8_UNLOCK();

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeError(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef out;
    *exception = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        Local<String> str = String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();

        if (argumentCount>0) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = VALUE(arguments[0])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                str = maybe.ToLocalChecked();
            } else {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        out = JSValue<Value>::New(context_, Exception::Error(str));
    V8_UNLOCK();

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeRegExp(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception)
{
    *exception = nullptr;
    JSObjectRef out = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        Local<String> pattern =
            String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();
        Local<String> flags_ =
            String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();

        if (argumentCount > 0) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = VALUE(arguments[0])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                pattern = maybe.ToLocalChecked();
            } else {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!*exception && argumentCount > 1) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = VALUE(arguments[1])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                flags_ = maybe.ToLocalChecked();
            } else {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!*exception) {
            String::Utf8Value const str(flags_);
            RegExp::Flags flags = RegExp::Flags::kNone;
            for (size_t i=0; i<strlen(*str); i++) {
                switch ((*str)[i]) {
                    case 'g': flags = (RegExp::Flags) (flags | RegExp::Flags::kGlobal);     break;
                    case 'i': flags = (RegExp::Flags) (flags | RegExp::Flags::kIgnoreCase); break;
                    case 'm': flags = (RegExp::Flags) (flags | RegExp::Flags::kMultiline);  break;
                }
            }

            TryCatch trycatch(isolate);

            MaybeLocal<RegExp> regexp = RegExp::New(context, pattern, flags);
            if (regexp.IsEmpty()) {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            } else {
                out = JSValue<Value>::New(context_, regexp.ToLocalChecked());
            }
        }

    V8_UNLOCK();

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeFunction(JSContextRef ctx, JSStringRef name,
    unsigned parameterCount, const JSStringRef parameterNames[], JSStringRef body,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception)
{
    *exception = nullptr;
    JSObjectRef out = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context);
        TryCatch trycatch(isolate);

        Local<String> source = String::NewFromUtf8(isolate, "(function(");
        Local<String> comma = String::NewFromUtf8(isolate, ",");
        for (unsigned i=0; i<parameterCount; i++) {
            source = String::Concat(source, parameterNames[i]->Value());
            if (i+1 < parameterCount) {
                source = String::Concat(source, comma);
            }
        }
        source = String::Concat(source, String::NewFromUtf8(isolate, "){\n"));
        source = String::Concat(source, body->Value());
        source = String::Concat(source, String::NewFromUtf8(isolate, "\n})"));

        ScriptOrigin script_origin(
            sourceURL->Value(),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Script> script = Script::Compile(context, source, &script_origin);
        if (script.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        MaybeLocal<Value> result;

        if (!*exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                *exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!*exception) {
            Local<Function> function = Local<Function>::Cast(result.ToLocalChecked());
            function->SetName(name->Value());
            out = JSValue<Value>::New(context_, result.ToLocalChecked());
        }
    V8_UNLOCK();

    return out;
}

JS_EXPORT JSValueRef JSObjectGetPrototype(JSContextRef ctx, JSObjectRef object)
{
    JSValueRef out;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        out = JSValue<Value>::New(context_, o->GetPrototype());
    V8_UNLOCK();

    return out;
}

JS_EXPORT void JSObjectSetPrototype(JSContextRef ctx, JSObjectRef object, JSValueRef value)
{
    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        o->SetPrototype(context, VALUE(value)->Value());
    V8_UNLOCK();
}

JS_EXPORT bool JSObjectHasProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    bool v;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        Maybe<bool> has = o->Has(context, propertyName->Value());
        v = has.FromMaybe(false);
    V8_UNLOCK();

    return v;
}

JS_EXPORT JSValueRef JSObjectGetProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exception)
{
    JSValueRef out = nullptr;
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, propertyName->Value());
        if (value.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!*exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }
    V8_UNLOCK();

    return out;
}

JS_EXPORT void JSObjectSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
    JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exception)
{
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        int v8_attr = v8::None;
        if (attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if (attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if (attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        TryCatch trycatch(isolate);

        Maybe<bool> defined = (attributes!=0) ?
            o->DefineOwnProperty(
                context,
                propertyName->Value(),
                VALUE(value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, propertyName->Value(), VALUE(value)->Value());

        if (defined.IsNothing()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }
    V8_UNLOCK();
}

JS_EXPORT bool JSObjectDeleteProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exception)
{
    bool v = false;
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        TryCatch trycatch(isolate);

        Maybe<bool> deleted = o->Delete(context, propertyName->Value());
        if (deleted.IsNothing()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        } else {
            v = deleted.FromMaybe(false);
        }
    V8_UNLOCK();

    return v;
}

JS_EXPORT JSValueRef JSObjectGetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef* exception)
{
    JSValueRef out = nullptr;
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, propertyIndex);
        if (value.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!*exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }
    V8_UNLOCK();

    return out;
}

JS_EXPORT void JSObjectSetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef value, JSValueRef* exception)
{
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        TryCatch trycatch(isolate);

        Maybe<bool> defined =
            o->Set(context, propertyIndex, VALUE(value)->Value());

        if (defined.IsNothing()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }
    V8_UNLOCK();
}

JS_EXPORT void* JSObjectGetPrivate(JSObjectRef object)
{
    void *data = nullptr;

    V8_ISOLATE_OBJ(object->Context(),object,isolate,context,o);
        Local<Private> privateKey = v8::Private::ForApi(isolate,
            String::NewFromUtf8(isolate, "__private"));
        Local<Value> payload;
        bool has = o->GetPrivate(context, privateKey).ToLocal(&payload);
        if (has && payload->IsNumber()) {
            data = (void *)((long)payload->ToNumber(context).ToLocalChecked()->Value());
        }
    V8_UNLOCK();

    return data;
}

JS_EXPORT bool JSObjectSetPrivate(JSObjectRef object, void* data)
{
    V8_ISOLATE_OBJ(object->Context(),object,isolate,context,o);
        Local<Private> privateKey = v8::Private::ForApi(isolate,
            String::NewFromUtf8(isolate, "__private"));
        o->SetPrivate(context, privateKey,
            Number::New(isolate,(double)reinterpret_cast<long>(data)));
    V8_UNLOCK();

    return true;
}

JS_EXPORT bool JSObjectIsFunction(JSContextRef ctx, JSObjectRef object)
{
    bool v;

    VALUE_ISOLATE(CTX(ctx),object,isolate,context,value);
        v = value->IsFunction();
    V8_UNLOCK();

    return v;
}

JS_EXPORT JSValueRef JSObjectCallAsFunction(JSContextRef ctx, JSObjectRef object,
    JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[],
    JSValueRef* exception)
{
    JSValueRef out = nullptr;
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        Local<Value> this_ = thisObject ?
            VALUE(thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        Local<Value> *elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            elements[i] = VALUE(arguments[i])->Value();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, argumentCount, elements);
        if (value.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!*exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }
        delete [] elements;
    V8_UNLOCK();

    return out;
}

JS_EXPORT bool JSObjectIsConstructor(JSContextRef ctx, JSObjectRef object)
{
    return JSObjectIsFunction(ctx, object);
}

JS_EXPORT JSObjectRef JSObjectCallAsConstructor(JSContextRef ctx, JSObjectRef object,
    size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef out = nullptr;
    *exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        Local<Value> *elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            elements[i] = VALUE(arguments[i])->Value();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsConstructor(context, argumentCount, elements);
        if (value.IsEmpty()) {
            *exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!*exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }
        delete [] elements;
    V8_UNLOCK();

    return out;
}

JS_EXPORT JSPropertyNameArrayRef JSObjectCopyPropertyNames(JSContextRef ctx, JSObjectRef object)
{
    JSPropertyNameArrayRef array;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o);
        Local<Array> names = o->GetPropertyNames(context).ToLocalChecked();
        array = JSValue<Array>::New(context_, names);
    V8_UNLOCK();

    return array;
}

JS_EXPORT JSPropertyNameArrayRef JSPropertyNameArrayRetain(JSPropertyNameArrayRef array)
{
    V8_ISOLATE_CTX(array->Context(),isolate,context);
        array->retain();
    V8_UNLOCK();

    return array;
}

JS_EXPORT void JSPropertyNameArrayRelease(JSPropertyNameArrayRef array)
{
    V8_ISOLATE_CTX(array->Context(),isolate,context);
        array->release();
    V8_UNLOCK();
}

JS_EXPORT size_t JSPropertyNameArrayGetCount(JSPropertyNameArrayRef array)
{
    size_t size;

    V8_ISOLATE_CTX(array->Context(),isolate,context);
        size = array->Value()->Length();
    V8_UNLOCK();

    return size;
}

JS_EXPORT JSStringRef JSPropertyNameArrayGetNameAtIndex(JSPropertyNameArrayRef array, size_t index)
{
    JSStringRef ret = nullptr;

    V8_ISOLATE_CTX(array->Context(),isolate,context);
        if (index < array->Value()->Length()) {
            Local<Value> element = array->Value()->Get(context, index).ToLocalChecked();
            String::Utf8Value const str(element->ToString(context).ToLocalChecked());
            ret = JSStringCreateWithUTF8CString(*str);
        }
    V8_UNLOCK();

    return ret;
}

JS_EXPORT void JSPropertyNameAccumulatorAddName(JSPropertyNameAccumulatorRef accumulator,
    JSStringRef propertyName)
{
    accumulator->push_front(propertyName);
}
