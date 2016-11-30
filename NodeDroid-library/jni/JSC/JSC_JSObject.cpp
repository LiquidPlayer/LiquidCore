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
    Local<Value> value = (reinterpret_cast<JSValue<Value>*>(valueRef))->Value();

#define V8_ISOLATE_CALLBACK(info,isolate,context,definition) \
    Isolate::Scope isolate_scope_(info.GetIsolate()); \
    HandleScope handle_scope_(info.GetIsolate()); \
    Local<Object> obj_ = info.Data()->ToObject(); \
    const JSClassDefinition *definition = \
        (JSClassDefinition*) obj_->GetAlignedPointerFromInternalField(0);\
    if (nullptr == info.Data()->ToObject()->GetAlignedPointerFromInternalField(1)) return; \
    JSContextRef ctxRef_ = (JSContextRef)obj_->GetAlignedPointerFromInternalField(1); \
    V8_ISOLATE_CTX(ctxRef_->Context(),isolate,context)

#define VALUE(value) ((JSValue<Value> *)(value))
#define CTX(ctx)     ((ctx)->Context())

OpaqueJSClass::OpaqueJSClass(const JSClassDefinition *definition)
{
    m_definition = new JSClassDefinition;
    memcpy(m_definition, definition, sizeof(JSClassDefinition));

    if(m_definition->parentClass) {
        m_definition->parentClass->retain();
    }
}

OpaqueJSClass::~OpaqueJSClass()
{
    if (m_definition->parentClass) {
        m_definition->parentClass->release();
    }

    delete m_definition;
}

void OpaqueJSClass::StaticFunctionCallHandler(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
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
            for (int i=0; !value && !exception &&
                definition->staticFunctions && definition->staticFunctions[i].name;
                i++) {

                if (!strcmp(definition->staticFunctions[i].name, *str) &&
                    definition->staticFunctions[i].callAsFunction) {

                    value = definition->staticFunctions[i].callAsFunction(
                        ctxRef_,
                        function,
                        thisObject,
                        (size_t) info.Length(),
                        arguments,
                        &exception);
                }
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

    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyQuerier(Local< String > property,
    const PropertyCallbackInfo< Integer > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        bool has = false;

        while (definition && !has) {
            if (definition->hasProperty) {
                has = definition->hasProperty(
                    ctxRef_,
                    thisObject,
                    string);
            }

            if (!has) {
                // check static values
                for (int i=0; !has && definition->staticValues && definition->staticValues[i].name;
                    i++) {

                    if (!strcmp(definition->staticValues[i].name, *str) &&
                        !(definition->staticValues[i].attributes & kJSPropertyAttributeDontEnum)) {
                        has = true;
                    }
                }
                // check static functions
                for (int i=0; !has && definition->staticFunctions &&
                    definition->staticFunctions[i].name;
                    i++) {

                    if (!strcmp(definition->staticFunctions[i].name, *str) &&
                        !(definition->staticFunctions[i].attributes &
                        kJSPropertyAttributeDontEnum)) {
                        has = true;
                    }
                }
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        thisObject->release();
        string->release();

        if (has) {
            info.GetReturnValue().Set(v8::None);
        }
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyGetter(Local< String > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        JSValueRef exception = nullptr;

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        JSValueRef value = nullptr;

        while (definition && !exception && !value) {
            if (definition->getProperty) {
                value = definition->getProperty(
                    ctxRef_,
                    thisObject,
                    string,
                    &exception);
            }

            // Check static values
            for (int i=0; !value && !exception &&
                definition->staticValues && definition->staticValues[i].name;
                i++) {

                if (!strcmp(definition->staticValues[i].name, *str) &&
                    definition->staticValues[i].getProperty) {

                    value = definition->staticValues[i].getProperty(
                        ctxRef_,
                        thisObject,
                        string,
                        &exception);
                }
            }

            // Check static functions
            for (int i=0; !value && !exception &&
                definition->staticFunctions && definition->staticFunctions[i].name;
                i++) {

                if (!strcmp(definition->staticFunctions[i].name, *str) &&
                    definition->staticFunctions[i].callAsFunction) {

                    Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
                    templ->SetInternalFieldCount(2);
                    Local<Object> data = templ->NewInstance(context).ToLocalChecked();
                    data->SetAlignedPointerInInternalField(0,(void*)definition);
                    data->SetAlignedPointerInInternalField(1,(void*)ctxRef_);
                    data->Set(context, String::NewFromUtf8(isolate, "name"), property);
                    //context_->retain();
                    UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, data);
                    weak->SetWeak<UniquePersistent<Object>>(
                        weak,
                        [](const WeakCallbackInfo<UniquePersistent<Object>>& info) {

                        //JSContext* ctx = reinterpret_cast<JSContext*>(info.GetInternalField(1));
                        //ctx->release();
                        info.GetParameter()->Reset();
                        delete info.GetParameter();
                    }, v8::WeakCallbackType::kInternalFields);

                    Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
                        StaticFunctionCallHandler, data);
                    Local<Function> func = ftempl->GetFunction();
                    data->Set(context, String::NewFromUtf8(isolate, "func"), func);
                    value = JSValue<Value>::New(context_, func);
                }
            }

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
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertySetter(Local< String > property, Local< Value > value,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        JSValueRef exception = nullptr;

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());
        JSValueRef valueRef = JSValue<Value>::New(context_,value);

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        bool set = false;
        while (definition && !exception && !set) {
            if (definition->setProperty) {
                set = definition->setProperty(
                    ctxRef_,
                    thisObject,
                    string,
                    valueRef,
                    &exception);
            }

            // Check static values
            for (int i=0; !set && !exception &&
                definition->staticValues && definition->staticValues[i].name;
                i++) {

                if (!strcmp(definition->staticValues[i].name, *str) &&
                    definition->staticValues[i].getProperty) {

                    set = definition->staticValues[i].setProperty(
                        ctxRef_,
                        thisObject,
                        string,
                        valueRef,
                        &exception);
                }
            }

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
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyDeleter(Local< String > property,
    const PropertyCallbackInfo< Boolean > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        JSValueRef exception = nullptr;

        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        String::Utf8Value const str(property);
        JSStringRef string = JSStringCreateWithUTF8CString(*str);

        bool deleted = false;
        while (definition && !exception && !deleted) {
            if (definition->deleteProperty) {
                deleted = definition->deleteProperty(
                    ctxRef_,
                    thisObject,
                    string,
                    &exception);
            }
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
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyEnumerator(const PropertyCallbackInfo< Array > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        JSObjectRef thisObject = JSValue<Value>::New(context_, info.This());

        OpaqueJSPropertyNameAccumulator accumulator;
        if (definition->getPropertyNames) {
            definition->getPropertyNames(
                ctxRef_,
                thisObject,
                &accumulator);
        }

        Local<Array> array = Array::New(isolate, accumulator.size());
        for(size_t i=0; accumulator.size() > 0; i++) {
            array->Set(context, i, accumulator.back()->Value(isolate));
            accumulator.back()->release();
            accumulator.pop_back();
        }

        info.GetReturnValue().Set(array);

        thisObject->release();
    V8_UNLOCK()
}

void OpaqueJSClass::CallAsFunction(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
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
                value = definition->callAsConstructor(
                    ctxRef_,
                    function,
                    (size_t) info.Length(),
                    arguments,
                    &exception);
                if (!exception) {
                    // FIXME: This shallow copy doesn't do exactly what JSC expects
                    // JSC expects to return the instance object.  However, V8 wants to create the
                    // instance object for you, which you can modify.  To deal with this, we are
                    // simply doing a shallow copy of properties from the JSC returned instance
                    // object to the V8 assigned instance object.  In the simplest cases, this will
                    // probably have equivalent functionality.  But non-enumerable properties will
                    // not get copied, read-only properties will become read/write, and any explicit
                    // expectations of JSObjectRefs being equal will not be true.  There are likely
                    // other unintended consequences as well.
                    Local<Object> OBJECT =
                        context->Global()->Get(String::NewFromUtf8(isolate, "Object"))->ToObject();
                    Local<Function> assign =
                        OBJECT->Get(String::NewFromUtf8(isolate, "assign")).As<Function>();
                    Local<Value> *arguments = new Local<Value>[2];
                    arguments[1] = info.This();
                    arguments[2] = VALUE(value)->Value();
                    assign->Call(Local<Value>::New(isolate,Null(isolate)), 2, arguments);
                    delete [] arguments;
                }
            } else if (!info.IsConstructCall() && definition->callAsFunction) {
                value = definition->callAsFunction(
                    ctxRef_,
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
    V8_UNLOCK()
}

void OpaqueJSClass::Finalize(const WeakCallbackInfo<UniquePersistent<Object>>& info)
{
    __android_log_print(ANDROID_LOG_DEBUG, "Finalize", "Are we getting called?");
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

Local<ObjectTemplate> OpaqueJSClass::NewTemplate(Local<Object> *data)
{
    Isolate *isolate = Isolate::GetCurrent();
    Local<Context> temp = Context::New(isolate);

    Local<ObjectTemplate> object = ObjectTemplate::New(isolate);

    // Create data object
    Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
    templ->SetInternalFieldCount(2);
    *data = templ->NewInstance(temp).ToLocalChecked();
    (*data)->SetAlignedPointerInInternalField(0,(void*)m_definition);
    (*data)->SetAlignedPointerInInternalField(1,nullptr);
    UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, *data);
    weak->SetWeak<UniquePersistent<Object>>(
        weak,
        [](const WeakCallbackInfo<UniquePersistent<Object>>& info) {

        //JSContext* ctx = reinterpret_cast<JSContext*>(info.GetInternalField(1));
        //ctx->release();
        info.GetParameter()->Reset();
        delete info.GetParameter();
    }, v8::WeakCallbackType::kInternalFields);

    object->SetNamedPropertyHandler(
        NamedPropertyGetter,
        NamedPropertySetter,
        NamedPropertyQuerier,
        NamedPropertyDeleter,
        NamedPropertyEnumerator,
        *data);

    if (m_definition->callAsFunction || m_definition->callAsConstructor) {
        object->SetCallAsFunctionHandler(CallAsFunction, *data);
    }

    // FIXME: hasInstance not implemented
    // FIXME: convertToType not implemented

    object->SetInternalFieldCount(1);

    return object;
}

JSValueRef OpaqueJSClass::InitInstance(JSContextRef ctx, Local<Object> instance, Local<Object> data)
{
    JSValue<Value> *retObj;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        data->SetAlignedPointerInInternalField(1,(void*)ctx);
        //((OpaqueJSContext*)ctx)->retain();

        if (m_definition->callAsFunction || m_definition->callAsConstructor) {
            data->Set(CTX(ctx)->Value(), String::NewFromUtf8(isolate, "func"), instance);
        }
        retObj = JSValue<Value>::New(ctx->Context(), instance);

        UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, instance);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            Finalize,
            v8::WeakCallbackType::kInternalFields);

        /*
        if (instance->InternalFieldCount()) {
            instance->SetAlignedPointerInInternalField(0,this);
        } else {
            Local<Object> prototype = instance->GetPrototype()->ToObject(context).ToLocalChecked();
            prototype->SetAlignedPointerInInternalField(0,this);
        }
        */
        instance->SetAlignedPointerInInternalField(0,this);
        retain();

        // Find the greatest ancestor
        const JSClassDefinition *definition = nullptr;
        for (definition = m_definition; definition && definition->parentClass;
            definition = definition->parentClass->m_definition);

        // Walk backwards and call 'initialize on each'
        while (true) {
            if (definition->initialize)
                definition->initialize(ctx, retObj);
            const JSClassDefinition *parent = definition;
            if (parent == m_definition) break;

            for (definition = m_definition;
                definition->parentClass && definition->parentClass->m_definition != parent;
                definition = definition->parentClass->m_definition);
        }
    V8_UNLOCK()

    return retObj;
}

const JSClassDefinition kJSClassDefinitionEmpty = {
    0, 0,
    nullptr, nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

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

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        if (jsClass) {
            Local<Object> payload;
            Local<ObjectTemplate> templ = jsClass->NewTemplate(&payload);
            Local<Object> instance = templ->NewInstance(context).ToLocalChecked();
            Local<Private> privateKey = v8::Private::ForApi(isolate,
                String::NewFromUtf8(isolate, "__private"));
            instance->SetPrivate(context, privateKey,
                Number::New(isolate,(double)reinterpret_cast<long>(data)));
            value = VALUE(jsClass->InitInstance(ctx, instance, payload));
        } else {
            value = JSValue<Value>::New(context_, Object::New(isolate));
            //JSObjectSetPrivate(value, data);
        }
    V8_UNLOCK()

    return value;
}

static JSObjectRef SetUpFunction(JSContextRef ctx, JSStringRef name, JSClassDefinition *definition)
{
    JSObjectRef obj;
    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
        templ->SetInternalFieldCount(2);
        Local<Object> data = templ->NewInstance(context).ToLocalChecked();
        data->SetAlignedPointerInInternalField(0,definition);
        data->SetAlignedPointerInInternalField(1,(void*)ctx);
        //context_->retain();
        UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, data);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            [](const WeakCallbackInfo<UniquePersistent<Object>>& info) {

            //JSContextRef ctx = reinterpret_cast<JSContextRef>(info.GetInternalField(1));
            JSClassDefinition *definition =
                reinterpret_cast<JSClassDefinition*>(info.GetInternalField(0));
            delete definition;
            //((JSContext*)ctx)->release();
            info.GetParameter()->Reset();
            delete info.GetParameter();
        }, v8::WeakCallbackType::kInternalFields);

        Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
            OpaqueJSClass::CallAsFunction, data);
        Local<Function> func = ftempl->GetFunction();

        if (name) {
            func->SetName(name->Value(isolate));
        }

        data->Set(context, String::NewFromUtf8(isolate, "func"), func);

        obj = JSValue<Value>::New(context_, func);
    V8_UNLOCK()

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

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<Array> array = Array::New(isolate, argumentCount);
        for(size_t i=0; i<argumentCount; i++) {
            array->Set(context, i, VALUE(arguments[i])->Value());
        }
        object = JSValue<Value>::New(context_, array);
    V8_UNLOCK()

    return object;
}

JS_EXPORT JSObjectRef JSObjectMakeDate(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    JSObjectRef out;
    JSValueRef exception = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
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
                exception = JSValue<Value>::New(context_, trycatch.Exception());
                epoch = 0.0;
            }

            date = Date::New(isolate, epoch);
        }

        out = JSValue<Value>::New(context_, date);

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeError(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    JSObjectRef out;
    JSValueRef exception = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<String> str =
            String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();

        if (argumentCount>0) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = VALUE(arguments[0])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                str = maybe.ToLocalChecked();
            } else {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        out = JSValue<Value>::New(context_, Exception::Error(str));

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeRegExp(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    JSValueRef exception = nullptr;
    JSObjectRef out = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
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
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!exception && argumentCount > 1) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = VALUE(arguments[1])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                flags_ = maybe.ToLocalChecked();
            } else {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!exception) {
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
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            } else {
                out = JSValue<Value>::New(context_, regexp.ToLocalChecked());
            }
        }

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeFunction(JSContextRef ctx, JSStringRef name,
    unsigned parameterCount, const JSStringRef parameterNames[], JSStringRef body,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exceptionRef)
{
    JSValueRef exception = nullptr;
    JSObjectRef out = nullptr;

    OpaqueJSString anonymous("anonymous");

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        TryCatch trycatch(isolate);

        Local<String> source = String::NewFromUtf8(isolate, "(function ");
        if (name) {
            source = String::Concat(source, name->Value(isolate));
        }
        source = String::Concat(source, String::NewFromUtf8(isolate, "("));
        Local<String> comma = String::NewFromUtf8(isolate, ",");
        for (unsigned i=0; i<parameterCount; i++) {
            source = String::Concat(source, parameterNames[i]->Value(isolate));
            if (i+1 < parameterCount) {
                source = String::Concat(source, comma);
            }
        }
        source = String::Concat(source, String::NewFromUtf8(isolate, ") { "));
        if (body) {
            source = String::Concat(source, body->Value(isolate));
        }
        source = String::Concat(source, String::NewFromUtf8(isolate, "\n})"));

        ScriptOrigin script_origin(
            sourceURL ? sourceURL->Value(isolate) : anonymous.Value(isolate),
            Integer::New(isolate, startingLineNumber)
        );

        MaybeLocal<Script> script = Script::Compile(context, source, &script_origin);
        if (script.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        MaybeLocal<Value> result;

        if (!exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                exception = JSValue<Value>::New(context_, trycatch.Exception());
            }
        }

        if (!exception) {
            Local<Function> function = Local<Function>::Cast(result.ToLocalChecked());
            if (name) {
                function->SetName(name->Value(isolate));
            }
            out = JSValue<Value>::New(context_, result.ToLocalChecked());
        }

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;
}

JS_EXPORT JSValueRef JSObjectGetPrototype(JSContextRef ctx, JSObjectRef object)
{
    JSValueRef out;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        out = JSValue<Value>::New(context_, o->GetPrototype());
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetPrototype(JSContextRef ctx, JSObjectRef object, JSValueRef value)
{
    JSValueRef null = JSValueMakeNull(ctx);
    if (!value) value = null;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        o->SetPrototype(context, VALUE(value)->Value());
        VALUE(null)->release();
    V8_UNLOCK()
}

JS_EXPORT bool JSObjectHasProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName)
{
    if (!propertyName) return false;
    bool v;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        Maybe<bool> has = o->Has(context, propertyName->Value(isolate));
        v = has.FromMaybe(false);
    V8_UNLOCK()

    return v;
}

JS_EXPORT JSValueRef JSObjectGetProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exceptionRef)
{
    JSValueRef out = nullptr;
    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, propertyName->Value(isolate));
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
    JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exceptionRef)
{
    JSValueRef null = JSValueMakeNull(ctx);
    if (!value) value = null;

    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        int v8_attr = v8::None;
        if (attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if (attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if (attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        TryCatch trycatch(isolate);

        Maybe<bool> defined = (attributes!=0) ?
            o->DefineOwnProperty(
                context,
                propertyName->Value(isolate),
                VALUE(value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, propertyName->Value(isolate), VALUE(value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        VALUE(null)->release();

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()
}

JS_EXPORT bool JSObjectDeleteProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exceptionRef)
{
    if (!propertyName) return false;

    bool v = false;
    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TryCatch trycatch(isolate);

        Maybe<bool> deleted = o->Delete(context, propertyName->Value(isolate));
        if (deleted.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        } else {
            v = deleted.FromMaybe(false);
        }

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return v;
}

JS_EXPORT JSValueRef JSObjectGetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef* exceptionRef)
{
    JSValueRef out = nullptr;
    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, propertyIndex);
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef value, JSValueRef* exceptionRef)
{
    JSValueRef null = JSValueMakeNull(ctx);
    if (!value) value = null;

    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TryCatch trycatch(isolate);

        Maybe<bool> defined =
            o->Set(context, propertyIndex, VALUE(value)->Value());

        if (defined.IsNothing()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        VALUE(null)->release();

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()
}

JS_EXPORT void* JSObjectGetPrivate(JSObjectRef object)
{
    void *data = nullptr;

    V8_ISOLATE_OBJ(object->Context(),object,isolate,context,o)
        Local<Private> privateKey = v8::Private::ForApi(isolate,
            String::NewFromUtf8(isolate, "__private"));
        Local<Value> payload;
        bool has = o->GetPrivate(context, privateKey).ToLocal(&payload);
        if (has && payload->IsNumber()) {
            data = (void *)((long)payload->ToNumber(context).ToLocalChecked()->Value());
        }
    V8_UNLOCK()

    return data;
}

JS_EXPORT bool JSObjectSetPrivate(JSObjectRef object, void* data)
{
    bool has = false;

    V8_ISOLATE_OBJ(object->Context(),object,isolate,context,o)
        Local<Value> payload;
        Local<Private> privateKey = v8::Private::ForApi(isolate,
            String::NewFromUtf8(isolate, "__private"));
        has = o->GetPrivate(context, privateKey).ToLocal(&payload);
        if (has && payload->IsNumber()) {
            o->SetPrivate(context, privateKey,
                Number::New(isolate,(double)reinterpret_cast<long>(data)));
        } else {
            has = false;
        }
    V8_UNLOCK()

    return has;
}

JS_EXPORT bool JSObjectIsFunction(JSContextRef ctx, JSObjectRef object)
{
    if (!object) return false;
    bool v;

    VALUE_ISOLATE(CTX(ctx),object,isolate,context,value)
        v = value->IsFunction();
    V8_UNLOCK()

    return v;
}

JS_EXPORT JSValueRef JSObjectCallAsFunction(JSContextRef ctx, JSObjectRef object,
    JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[],
    JSValueRef* exceptionRef)
{
    if (!object) return nullptr;

    JSValueRef out = nullptr;
    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        Local<Value> this_ = thisObject ?
            VALUE(thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        Local<Value> *elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            if (arguments[i]) {
                elements[i] = VALUE(arguments[i])->Value();
            } else {
                elements[i] = Local<Value>::New(isolate,Null(isolate));
            }
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, argumentCount, elements);
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }
        delete [] elements;

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;
}

JS_EXPORT bool JSObjectIsConstructor(JSContextRef ctx, JSObjectRef object)
{
    return JSObjectIsFunction(ctx, object);
}

JS_EXPORT JSObjectRef JSObjectCallAsConstructor(JSContextRef ctx, JSObjectRef object,
    size_t argumentCount, const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    if (!object) return nullptr;

    JSObjectRef out = nullptr;
    JSValueRef exception = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        Local<Value> *elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            elements[i] = VALUE(arguments[i])->Value();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsConstructor(context, argumentCount, elements);
        if (value.IsEmpty()) {
            exception = JSValue<Value>::New(context_, trycatch.Exception());
        }

        if (!exception) {
            out = JSValue<Value>::New(context_, value.ToLocalChecked());
        }
        delete [] elements;

        if (exceptionRef) *exceptionRef = exception;
        else if (exception) VALUE(exception)->release();
    V8_UNLOCK()

    return out;
}

JS_EXPORT JSPropertyNameArrayRef JSObjectCopyPropertyNames(JSContextRef ctx, JSObjectRef object)
{
    if (!object) return nullptr;

    JSPropertyNameArrayRef array;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        Local<Array> names = o->GetPropertyNames(context).ToLocalChecked();
        JSClassRef classRef = nullptr;

        if (o->InternalFieldCount()) {
            classRef = (JSClassRef) o->GetAlignedPointerFromInternalField(0);
        } else if (!o->GetPrototype()->ToObject(context).IsEmpty()) {
            Local<Object> prototype = o->GetPrototype()->ToObject(context).ToLocalChecked();
            if (prototype->InternalFieldCount()) {
                classRef = (JSClassRef) prototype->GetAlignedPointerFromInternalField(0);
            }
        }
        /*
        if (o->InternalFieldCount()) {
            classRef = (JSClassRef) o->GetAlignedPointerFromInternalField(0);
        }
        */

        if (classRef) {
            __android_log_print(ANDROID_LOG_DEBUG, "PropertyNames", "need to add some shit");
            // Add static values
            for (int i=0; classRef->Definition()->staticValues &&
                classRef->Definition()->staticValues[i].name; i++) {

                if (!(classRef->Definition()->staticValues[i].attributes &
                    kJSPropertyAttributeDontEnum)) {

                    names->Set(context, names->Length(), String::NewFromUtf8(isolate,
                        classRef->Definition()->staticValues[i].name));
                }
            }
            // Add static functions
            for (int i=0; classRef->Definition()->staticFunctions &&
                classRef->Definition()->staticFunctions[i].name; i++) {

                if (!(classRef->Definition()->staticFunctions[i].attributes &
                    kJSPropertyAttributeDontEnum)) {

                    names->Set(context, names->Length(), String::NewFromUtf8(isolate,
                        classRef->Definition()->staticFunctions[i].name));
                }
            }
        }
        array = JSValue<Array>::New(context_, names);
    V8_UNLOCK()

    return array;
}

JS_EXPORT JSPropertyNameArrayRef JSPropertyNameArrayRetain(JSPropertyNameArrayRef array)
{
    if (!array) return nullptr;

    V8_ISOLATE_CTX(array->Context(),isolate,context)
        array->retain();
    V8_UNLOCK()

    return array;
}

JS_EXPORT void JSPropertyNameArrayRelease(JSPropertyNameArrayRef array)
{
    if (!array) return;

    V8_ISOLATE_CTX(array->Context(),isolate,context)
        array->release();
    V8_UNLOCK()
}

JS_EXPORT size_t JSPropertyNameArrayGetCount(JSPropertyNameArrayRef array)
{
    if (!array) return 0;

    size_t size;

    V8_ISOLATE_CTX(array->Context(),isolate,context)
        size = array->Value()->Length();
    V8_UNLOCK()

    return size;
}

JS_EXPORT JSStringRef JSPropertyNameArrayGetNameAtIndex(JSPropertyNameArrayRef array, size_t index)
{
    if (!array) return nullptr;

    JSStringRef ret = nullptr;

    V8_ISOLATE_CTX(array->Context(),isolate,context)
        if (index < array->Value()->Length()) {
            Local<Value> element = array->Value()->Get(context, index).ToLocalChecked();
            String::Utf8Value const str(element->ToString(context).ToLocalChecked());
            ret = JSStringCreateWithUTF8CString(*str);
        }
    V8_UNLOCK()

    return ret;
}

JS_EXPORT void JSPropertyNameAccumulatorAddName(JSPropertyNameAccumulatorRef accumulator,
    JSStringRef propertyName)
{
    if (accumulator && propertyName) {
        accumulator->push_front(propertyName);
    }
}
