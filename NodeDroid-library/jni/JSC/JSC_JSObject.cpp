//
// Created by Eric on 11/6/16.
//

#include "JSC.h"

#define V8_ISOLATE_OBJ(ctx,object,isolate,context,o) \
    V8_ISOLATE_CTX(ctx,isolate,context); \
    Local<Object> o = \
        (*object)->Value()->ToObject(context).ToLocalChecked();

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> value = (*valueRef)->Value();

#define V8_ISOLATE_CALLBACK(info,isolate,context,definition) \
    Isolate::Scope isolate_scope_(info.GetIsolate()); \
    HandleScope handle_scope_(info.GetIsolate()); \
    Local<Object> obj_ = info.Data()->ToObject(); \
    const JSClassDefinition *definition = \
        (JSClassDefinition*) obj_->GetAlignedPointerFromInternalField(0);\
    if (nullptr == info.Data()->ToObject()->GetAlignedPointerFromInternalField(1)) return; \
    JSContextRef ctxRef_ = (JSContextRef)obj_->GetAlignedPointerFromInternalField(1); \
    V8_ISOLATE_CTX(ctxRef_->Context(),isolate,context)

#define TO_REAL_GLOBAL(o) \
    o = o->StrictEquals(context->Global()) && \
        !o->GetPrototype()->ToObject(context).IsEmpty() && \
        o->GetPrototype()->ToObject(context).ToLocalChecked()->InternalFieldCount() ? \
        o->GetPrototype()->ToObject(context).ToLocalChecked() : \
        o;


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
            arguments[i] = new OpaqueJSValue(ctxRef_, info[i]);
        }
        TempJSValue function(ctxRef_, obj_->Get(context,
            String::NewFromUtf8(isolate, "func")).ToLocalChecked());
        TempJSValue thisObject(ctxRef_, info.This());

        TempException exception(nullptr);
        TempJSValue value;

        while (definition && !*exception && !*value) {
            for (int i=0; !*value && !*exception &&
                definition->staticFunctions && definition->staticFunctions[i].name;
                i++) {

                if (!strcmp(definition->staticFunctions[i].name, *str) &&
                    definition->staticFunctions[i].callAsFunction) {

                    value.Set(definition->staticFunctions[i].callAsFunction(
                        ctxRef_,
                        const_cast<JSObjectRef>(*function),
                        const_cast<JSObjectRef>(*thisObject),
                        (size_t) info.Length(),
                        arguments,
                        &exception));
                }
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        for (int i=0; i<info.Length(); i++) {
            arguments[i]->Clean();
        }

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }

        if (*value) {
            info.GetReturnValue().Set((**value)->Value());
        }

    V8_UNLOCK()
}

void OpaqueJSClass::ConvertFunctionCallHandler(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        String::Utf8Value const str(info[0]);

        TempException exception(nullptr);
        TempJSValue value;

        JSType type =
            !strcmp("number",  *str) ? kJSTypeNumber :
            !strcmp("string", *str) ? kJSTypeString :
            kJSTypeNumber; // FIXME

        while (definition && !*exception && !*value) {

            if (definition->convertToType) {
                value.Set(definition->convertToType(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    type,
                    &exception));
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }


        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }

        if (*value) {
            info.GetReturnValue().Set((**value)->Value());
        }

    V8_UNLOCK()
}

void OpaqueJSClass::HasInstanceFunctionCallHandler(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());
        TempException exception(nullptr);
        TempJSValue value;
        TempJSValue possibleInstance(ctxRef_, info[0]);

        if (obj_->InternalFieldCount() > 2 && info[0]->IsObject() &&
            obj_->GetAlignedPointerFromInternalField(2)) {

            JSClassRef ctor = (JSClassRef)obj_-> GetAlignedPointerFromInternalField(2);
            if (info[0].As<Object>()->InternalFieldCount() > 0) {
                JSClassRef inst =
                    (JSClassRef) info[0].As<Object>()->GetAlignedPointerFromInternalField(0);
                bool has = false;
                for( ; inst && !has; inst = inst->Definition()->parentClass) {
                    has = ctor == inst;
                }
                value.Set(ctxRef_, Boolean::New(isolate, has));
            }
        }

        while (definition && !*exception && !*value) {

            if (definition->hasInstance) {
                bool has = definition->hasInstance(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    *possibleInstance,
                    &exception);
                value.Set(ctxRef_, Boolean::New(isolate, has));
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }

        if (*value) {
            info.GetReturnValue().Set((**value)->Value());
        }
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyQuerier(Local< String > property,
    const PropertyCallbackInfo< Integer > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        String::Utf8Value const str(property);
        OpaqueJSString string(*str);

        bool has = false;

        while (definition && !has) {
            if (definition->hasProperty) {
                has = definition->hasProperty(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    &string);
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
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (has) {
            info.GetReturnValue().Set(v8::DontEnum);
        }
    V8_UNLOCK()
}

void OpaqueJSClass::IndexedPropertyQuerier(uint32_t index,
    const PropertyCallbackInfo< Integer > &info)
{
    char prop[50];
    sprintf(prop, "%u", index);
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());
    NamedPropertyQuerier(String::NewFromUtf8(info.GetIsolate(),prop), info);
}

void OpaqueJSClass::ProtoPropertyQuerier(Local< String > property,
    const PropertyCallbackInfo< Integer > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        String::Utf8Value const str(property);

        bool has = false;

        while (definition && !has) {
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
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (has) {
            info.GetReturnValue().Set(v8::DontEnum);
        }
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyGetter(Local< String > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);
        TempJSValue thisObject(ctxRef_, info.This());
        TempJSValue value;

        String::Utf8Value const str(property);
        OpaqueJSString string(*str);

        const JSClassDefinition *top = definition;
        while (definition && !*value && !*exception) {
            // Check accessor
            bool hasProperty = true;
            if (definition->hasProperty) {
                hasProperty = definition->hasProperty(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    &string);
            }
            if (hasProperty && definition->getProperty) {
                value.Set(definition->getProperty(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    &string,
                    &exception));
            }

            // If this function returns NULL, the get request forwards to object's statically
            // declared properties ...
            // Check static values
            for (int i=0; !*value && !*exception &&
                definition->staticValues && definition->staticValues[i].name;
                i++) {

                if (!strcmp(definition->staticValues[i].name, *str) &&
                    definition->staticValues[i].getProperty) {

                    value.Set(definition->staticValues[i].getProperty(
                        ctxRef_,
                        const_cast<JSObjectRef>(*thisObject),
                        &string,
                        &exception));
                }
            }

            // then its parent class chain (which includes the default object class)
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (!*value && !*exception) {
            definition = top;
            while (definition && !*value && !*exception) {
                if (definition->hasProperty) {
                    bool has = definition->hasProperty(
                        ctxRef_,
                        const_cast<JSObjectRef>(*thisObject),
                        &string);
                    if (has) {
                        // Oops.  We claim to have this property, but we really don't
                        Local<String> error = String::NewFromUtf8(isolate, "Invalid property: ");
                        error = String::Concat(error, property);
                        exception.Set(ctxRef_, Exception::Error(error));
                    }
                }
                definition = definition->parentClass ?definition->parentClass->m_definition:nullptr;
            }
        }

        // ... then its prototype chain.

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }

        if (*value) {
            info.GetReturnValue().Set((**value)->Value());
        }
    V8_UNLOCK()
}

void OpaqueJSClass::IndexedPropertyGetter(uint32_t index,
    const PropertyCallbackInfo< Value > &info)
{
    char prop[50];
    sprintf(prop, "%u", index);
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());
    NamedPropertyGetter(String::NewFromUtf8(info.GetIsolate(),prop), info);
}

void OpaqueJSClass::ProtoPropertyGetter(Local< String > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);
        TempJSValue thisObject(ctxRef_, info.This());
        TempJSValue value;

        String::Utf8Value const str(property);
        OpaqueJSString string(*str);

        while (definition && !*value && !*exception) {
            auto valueFromFunction = [&](FunctionCallback function) {
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

                Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate, function, data);
                Local<Function> func = ftempl->GetFunction();
                data->Set(context, String::NewFromUtf8(isolate, "func"), func);
                value.Set(ctxRef_, func);
            };

            // Check static functions
            for (int i=0; !*value && !*exception &&
                definition->staticFunctions && definition->staticFunctions[i].name;
                i++) {

                if (!strcmp(definition->staticFunctions[i].name, *str) &&
                    definition->staticFunctions[i].callAsFunction) {

                    valueFromFunction(StaticFunctionCallHandler);
                }
            }

            // then its parent class chain (which includes the default object class)
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }

        if (*value) {
            info.GetReturnValue().Set((**value)->Value());
        }
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertySetter(Local< String > property, Local< Value > value,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);
        TempJSValue thisObject(ctxRef_, info.This());
        TempJSValue valueRef(ctxRef_,value);

        String::Utf8Value const str(property);
        OpaqueJSString string(*str);

        bool set = false;
        while (definition && !*exception && !set) {
            // Check static values
            for (int i=0; !set && !*exception &&
                definition->staticValues && definition->staticValues[i].name;
                i++) {

                if (!strcmp(definition->staticValues[i].name, *str) &&
                    definition->staticValues[i].setProperty) {

                    set = definition->staticValues[i].setProperty(
                        ctxRef_,
                        const_cast<JSObjectRef>(*thisObject),
                        &string,
                        *valueRef,
                        &exception);
                }
            }

            if( !set && !*exception) {
                if (definition->hasProperty) {
                    // Don't set real property if we are overriding accessors
                    set = definition->hasProperty(
                        ctxRef_,
                        const_cast<JSObjectRef>(*thisObject),
                        &string);
                }

                if (definition->setProperty) {
                    bool reset = definition->setProperty(
                        ctxRef_,
                        const_cast<JSObjectRef>(*thisObject),
                        &string,
                        *valueRef,
                        &exception);
                    set = set || reset;
                }
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
            value = (**exception)->Value();
            set = true;
        }

        if (set) {
            info.GetReturnValue().Set(value);
        }
    V8_UNLOCK()
}

void OpaqueJSClass::IndexedPropertySetter(uint32_t index, Local< Value > value,
    const PropertyCallbackInfo< Value > &info)
{
    char prop[50];
    sprintf(prop, "%u", index);
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());
    NamedPropertySetter(String::NewFromUtf8(info.GetIsolate(),prop), value, info);
}

void OpaqueJSClass::NamedPropertyDeleter(Local< String > property,
    const PropertyCallbackInfo< Boolean > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);

        TempJSValue thisObject(ctxRef_, info.This());

        String::Utf8Value const str(property);
        OpaqueJSString string(*str);

        bool deleted = false;
        while (definition && !*exception && !deleted) {
            if (definition->deleteProperty) {
                deleted = definition->deleteProperty(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    &string,
                    &exception);
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }
        if (deleted) {
            info.GetReturnValue().Set(deleted);
        }
    V8_UNLOCK()
}

void OpaqueJSClass::IndexedPropertyDeleter(uint32_t index,
    const PropertyCallbackInfo< Boolean > &info)
{
    char prop[50];
    sprintf(prop, "%u", index);
    Isolate::Scope isolate_scope_(info.GetIsolate());
    HandleScope handle_scope_(info.GetIsolate());
    NamedPropertyDeleter(String::NewFromUtf8(info.GetIsolate(),prop), info);
}

void OpaqueJSClass::NamedPropertyEnumerator(const PropertyCallbackInfo< Array > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        OpaqueJSPropertyNameAccumulator accumulator;
        while (definition) {
            if (definition->getPropertyNames) {
                definition->getPropertyNames(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    &accumulator);
            }

            // Add static values
            for (int i=0; definition->staticValues &&
                definition->staticValues[i].name; i++) {

                if (!(definition->staticValues[i].attributes &
                    kJSPropertyAttributeDontEnum)) {

                    JSStringRef property =
                        JSStringCreateWithUTF8CString(definition->staticValues[i].name);
                    JSPropertyNameAccumulatorAddName(&accumulator, property);
                    JSStringRelease(property);
                }
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        Local<Array> array = Array::New(isolate);
        Local<Function> indexOf = array->Get(context, String::NewFromUtf8(isolate, "indexOf"))
                .ToLocalChecked().As<Function>();
        Local<Function> push = array->Get(context, String::NewFromUtf8(isolate, "push"))
                .ToLocalChecked().As<Function>();
        while (accumulator.size() > 0) {
            Local<Value> property = accumulator.back()->Value(isolate);
            Local<Value> index = indexOf->Call(context, array, 1, &property).ToLocalChecked();
            if (index->ToNumber(context).ToLocalChecked()->Value() < 0) {
                push->Call(context, array, 1, &property);
            }
            accumulator.back()->release();
            accumulator.pop_back();
        }

        info.GetReturnValue().Set(array);
    V8_UNLOCK()
}

void OpaqueJSClass::IndexedPropertyEnumerator(const PropertyCallbackInfo< Array > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        OpaqueJSPropertyNameAccumulator accumulator;
        while (definition) {
            if (definition->getPropertyNames) {
                definition->getPropertyNames(
                    ctxRef_,
                    const_cast<JSObjectRef>(*thisObject),
                    &accumulator);
            }

            // Add static values
            for (int i=0; definition->staticValues &&
                definition->staticValues[i].name; i++) {

                if (!(definition->staticValues[i].attributes &
                    kJSPropertyAttributeDontEnum)) {

                    JSStringRef property =
                        JSStringCreateWithUTF8CString(definition->staticValues[i].name);
                    JSPropertyNameAccumulatorAddName(&accumulator, property);
                    JSStringRelease(property);
                }
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        Local<Array> array = Array::New(isolate);
        Local<Function> indexOf = array->Get(context, String::NewFromUtf8(isolate, "indexOf"))
                .ToLocalChecked().As<Function>();
        Local<Function> sort = array->Get(context, String::NewFromUtf8(isolate, "sort"))
                .ToLocalChecked().As<Function>();
        Local<Function> push = array->Get(context, String::NewFromUtf8(isolate, "push"))
                .ToLocalChecked().As<Function>();
        Local<Function> isNaN= context->Global()->Get(context, String::NewFromUtf8(isolate,"isNaN"))
                .ToLocalChecked().As<Function>();
        Local<Object> Number= context->Global()->Get(context, String::NewFromUtf8(isolate,"Number"))
                .ToLocalChecked()->ToObject(context).ToLocalChecked();
        Local<Function> isInteger = Number->Get(context, String::NewFromUtf8(isolate,"isInteger"))
                .ToLocalChecked().As<Function>();
        while (accumulator.size() > 0) {
            Local<Value> property = accumulator.back()->Value(isolate);
            Local<Value> numeric = property->ToNumber(context).ToLocalChecked();
            if (!isNaN->Call(context, isNaN, 1, &property).ToLocalChecked()
                ->ToBoolean(context).ToLocalChecked()->Value() &&
                isInteger->Call(context, isInteger, 1, &numeric).ToLocalChecked()
                ->ToBoolean(context).ToLocalChecked()->Value()) {

                Local<Value> index = indexOf->Call(context, array, 1,&numeric).ToLocalChecked();
                if (index->ToNumber(context).ToLocalChecked()->Value() < 0) {
                    push->Call(context, array, 1, &numeric);
                }
            }
            accumulator.back()->release();
            accumulator.pop_back();
        }
        sort->Call(context, array, 0, nullptr);

        info.GetReturnValue().Set(array);
    V8_UNLOCK()
}

void OpaqueJSClass::ProtoPropertyEnumerator(const PropertyCallbackInfo< Array > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        OpaqueJSPropertyNameAccumulator accumulator;
        while (definition) {
            // Add static functions
            for (int i=0; definition->staticFunctions &&
                definition->staticFunctions[i].name; i++) {

                if (!(definition->staticFunctions[i].attributes &
                    kJSPropertyAttributeDontEnum)) {

                    JSStringRef property =
                        JSStringCreateWithUTF8CString(definition->staticFunctions[i].name);
                    JSPropertyNameAccumulatorAddName(&accumulator, property);
                    JSStringRelease(property);
                }
            }

            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        Local<Array> array = Array::New(isolate);
        Local<Function> indexOf = array->Get(context, String::NewFromUtf8(isolate, "indexOf"))
                .ToLocalChecked().As<Function>();
        Local<Function> push = array->Get(context, String::NewFromUtf8(isolate, "push"))
                .ToLocalChecked().As<Function>();
        while (accumulator.size() > 0) {
            Local<Value> property = accumulator.back()->Value(isolate);
            Local<Value> index = indexOf->Call(context, array, 1, &property).ToLocalChecked();
            if (index->ToNumber(context).ToLocalChecked()->Value() < 0) {
                push->Call(context, array, 1, &property);
            }
            accumulator.back()->release();
            accumulator.pop_back();
        }

        info.GetReturnValue().Set(array);
    V8_UNLOCK()
}

void OpaqueJSClass::CallAsFunction(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);

        JSValueRef arguments[info.Length()];
        for (int i=0; i<info.Length(); i++) {
            arguments[i] = new OpaqueJSValue(ctxRef_, info[i]);
        }
        TempJSValue function(ctxRef_, obj_->Get(context,
            String::NewFromUtf8(isolate, "func")).ToLocalChecked());
        TempJSValue thisObject(ctxRef_, info.This());

        TempJSValue value;
        while (definition && !*exception && !*value) {
            if (info.IsConstructCall() && obj_->InternalFieldCount() > 2 &&
                obj_->GetAlignedPointerFromInternalField(2)) {
                value.Set(
                    JSObjectMake(ctxRef_, (JSClassRef)obj_->GetAlignedPointerFromInternalField(2),
                        nullptr));
            } else if (info.IsConstructCall() && definition->callAsConstructor) {
                value.Set(definition->callAsConstructor(
                    ctxRef_,
                    const_cast<JSObjectRef>(*function),
                    (size_t) info.Length(),
                    arguments,
                    &exception));
                if (!*value || !(**value)->Value()->IsObject()) {
                    value.Reset();
                    Local<String> error = String::NewFromUtf8(isolate, "Bad constructor");
                    exception.Set(ctxRef_, Exception::Error(error));
                }
            } else if (!info.IsConstructCall() && definition->callAsFunction) {
                value.Set(definition->callAsFunction(
                    ctxRef_,
                    const_cast<JSObjectRef>(*function),
                    const_cast<JSObjectRef>(*thisObject),
                    (size_t) info.Length(),
                    arguments,
                    &exception));
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        for (int i=0; i<info.Length(); i++) {
            arguments[i]->Clean();
        }

        if (*exception) {
            isolate->ThrowException((**exception)->Value());
        }

        if (*value) {
            info.GetReturnValue().Set((**value)->Value());
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

bool OpaqueJSClass::IsFunction()
{
    const JSClassDefinition *definition = m_definition;
    while (definition) {
        if (definition->callAsFunction) {
            return true;
        }
        definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
    }
    return false;
}

bool OpaqueJSClass::IsConstructor()
{
    const JSClassDefinition *definition = m_definition;
    while (definition) {
        if (definition->callAsConstructor) {
            return true;
        }
        definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
    }
    return false;
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

    object->SetIndexedPropertyHandler(
        IndexedPropertyGetter,
        IndexedPropertySetter,
        IndexedPropertyQuerier,
        IndexedPropertyDeleter,
        IndexedPropertyEnumerator,
        *data);

    if (IsFunction() || IsConstructor()) {
        object->SetCallAsFunctionHandler(CallAsFunction, *data);
    }

    object->SetInternalFieldCount(1);

    return object;
}

JSObjectRef OpaqueJSClass::InitInstance(JSContextRef ctx, Local<Object> instance, Local<Object> data)
{
    JSObjectRef retObj;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        data->SetAlignedPointerInInternalField(1,(void*)ctx);
        //((OpaqueJSContext*)ctx)->retain();

        if (IsFunction() || IsConstructor()) {
            data->Set(CTX(ctx)->Value(), String::NewFromUtf8(isolate, "func"), instance);
        }
        if (IsFunction()) {
            retObj = new OpaqueJSValue(ctx, instance.As<Function>());
        } else {
            retObj = new OpaqueJSValue(ctx, instance);
        }

        UniquePersistent<Object>* weak = new UniquePersistent<Object>(isolate, instance);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            Finalize,
            v8::WeakCallbackType::kInternalFields);

        instance->SetAlignedPointerInInternalField(0,this);
        retain();

        // Set up a prototype object to handle static functions
        Local<ObjectTemplate> protoTemplate = ObjectTemplate::New(isolate);
        protoTemplate->SetNamedPropertyHandler(
            ProtoPropertyGetter,
            nullptr,
            ProtoPropertyQuerier,
            nullptr,
            ProtoPropertyEnumerator,
            data);
        Local<Object> prototype = protoTemplate->NewInstance(context).ToLocalChecked();
        instance->SetPrototype(context, prototype);

        // Set className
        const JSClassDefinition *definition = m_definition;
        while (true) {
            const char* sClassName = definition ? definition->className : "CallbackObject";
            if (sClassName) {
                Local<String> className = String::NewFromUtf8(isolate, sClassName);
                Local<Object> Symbol =
                    context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject();
                Local<Value> toStringTag = Symbol->Get(String::NewFromUtf8(isolate, "toStringTag"));
                prototype->Set(context, toStringTag, className);
                break;
            }
            if (!definition) break;
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        // Override @@toPrimitive if convertToType set
        definition = m_definition;
        while (definition) {
            if (definition->convertToType) {
                Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
                    ConvertFunctionCallHandler, data);
                Local<Function> function = ftempl->GetFunction(context).ToLocalChecked();
                Local<Object> Symbol =
                    context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject();
                Local<Value> toPrimitive = Symbol->Get(String::NewFromUtf8(isolate, "toPrimitive"));
                prototype->Set(context, toPrimitive, function);
                break;
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        // Override @@hasInstance if hasInstance set
        definition = m_definition;
        while (definition) {
            if (definition->hasInstance) {
                Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
                    HasInstanceFunctionCallHandler, data);
                Local<Function> function = ftempl->GetFunction(context).ToLocalChecked();
                Local<Object> Symbol =
                    context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject();
                Local<Value> hasInstance = Symbol->Get(String::NewFromUtf8(isolate, "hasInstance"));
                prototype->Set(context, hasInstance, function);
                break;
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        // Find the greatest ancestor
        definition = nullptr;
        for (definition = m_definition; definition && definition->parentClass;
            definition = definition->parentClass->m_definition);

        // Walk backwards and call 'initialize' on each
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
    nullptr,
    nullptr, nullptr,
    nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr
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
            value = jsClass->InitInstance(ctx, instance, payload);
        } else {
            value = new OpaqueJSValue(ctx, Object::New(isolate));
        }
    V8_UNLOCK()

    return value;
}

static JSObjectRef SetUpFunction(JSContextRef ctx, JSStringRef name, JSClassDefinition *definition,
    JSClassRef jsClass, bool isConstructor)
{
    JSObjectRef obj;
    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
        templ->SetInternalFieldCount(3);
        Local<Object> data = templ->NewInstance(context).ToLocalChecked();
        data->SetAlignedPointerInInternalField(0,definition);
        data->SetAlignedPointerInInternalField(1,(void*)ctx);
        data->SetAlignedPointerInInternalField(2,(void*)jsClass);
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

        Local<Object> func;

        if (!isConstructor) {
            Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
                OpaqueJSClass::CallAsFunction, data);
            Local<Function> f = ftempl->GetFunction();
            if (name) {
                f->SetName(name->Value(isolate));
            }
            func = f;
        } else {
            Local<ObjectTemplate> ctempl = ObjectTemplate::New(isolate);
            ctempl->SetCallAsFunctionHandler(OpaqueJSClass::CallAsFunction, data);
            func = ctempl->NewInstance(context).ToLocalChecked();
        }

        if (jsClass) {
            Local<FunctionTemplate> ftempl = FunctionTemplate::New(isolate,
                OpaqueJSClass::HasInstanceFunctionCallHandler, data);
            Local<Function> function = ftempl->GetFunction(context).ToLocalChecked();
            Local<Object> Symbol =
                context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject();
            Local<Value> hasInstance = Symbol->Get(String::NewFromUtf8(isolate, "hasInstance"));
            Local<Object> prototype = Object::New(isolate);
            prototype->Set(context, hasInstance, function);
            func->SetPrototype(context, prototype);
        }

        data->Set(context, String::NewFromUtf8(isolate, "func"), func);

        obj = new OpaqueJSValue(ctx, func);
    V8_UNLOCK()

    return obj;
}

JS_EXPORT JSObjectRef JSObjectMakeFunctionWithCallback(JSContextRef ctx, JSStringRef name,
    JSObjectCallAsFunctionCallback callAsFunction)
{
    JSClassDefinition *definition = new JSClassDefinition;
    memset(definition, 0, sizeof(JSClassDefinition));
    definition->callAsFunction = callAsFunction;

    return SetUpFunction(ctx, name, definition, nullptr, false);
}

JS_EXPORT JSObjectRef JSObjectMakeConstructor(JSContextRef ctx, JSClassRef jsClass,
    JSObjectCallAsConstructorCallback callAsConstructor)
{
    JSClassDefinition *definition = new JSClassDefinition;
    memset(definition, 0, sizeof(JSClassDefinition));
    definition->callAsConstructor = callAsConstructor;

    return SetUpFunction(ctx, nullptr, definition, jsClass, true);
}

JS_EXPORT JSObjectRef JSObjectMakeArray(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception)
{
    JSObjectRef object;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<Array> array = Array::New(isolate, argumentCount);
        for(size_t i=0; i<argumentCount; i++) {
            array->Set(context, i, (*arguments[i])->Value());
        }
        object = new OpaqueJSValue(ctx, array);
    V8_UNLOCK()

    return object;
}

JS_EXPORT JSObjectRef JSObjectMakeDate(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    JSObjectRef out;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        TempException exception(exceptionRef);
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

            MaybeLocal<Number> number = (*arguments[0])->Value()->ToNumber(context);
            double epoch = 0.0;
            if (!number.IsEmpty()) {
                epoch = number.ToLocalChecked()->Value();
            } else {
                exception.Set(ctx, trycatch.Exception());
                epoch = 0.0;
            }

            date = Date::New(isolate, epoch);
        }

        out = new OpaqueJSValue(ctx, date);
    V8_UNLOCK()

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeError(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    JSObjectRef out;
    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        TempException exception(exceptionRef);

        Local<String> str =
            String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();

        if (argumentCount>0) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = (*arguments[0])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                str = maybe.ToLocalChecked();
            } else {
                exception.Set(ctx, trycatch.Exception());
            }
        }

        out = new OpaqueJSValue(ctx, Exception::Error(str));
    V8_UNLOCK()

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeRegExp(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exceptionRef)
{
    JSObjectRef out = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        TempException exception(exceptionRef);
        Local<String> pattern =
            String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();
        Local<String> flags_ =
            String::NewFromUtf8(isolate, "", NewStringType::kNormal).ToLocalChecked();

        if (argumentCount > 0) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = (*arguments[0])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                pattern = maybe.ToLocalChecked();
            } else {
                exception.Set(ctx, trycatch.Exception());
            }
        }

        if (!*exception && argumentCount > 1) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = (*arguments[1])->Value()->ToString(context);
            if (!maybe.IsEmpty()) {
                flags_ = maybe.ToLocalChecked();
            } else {
                exception.Set(ctx, trycatch.Exception());
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
                exception.Set(ctx, trycatch.Exception());
            } else {
                out = new OpaqueJSValue(ctx, regexp.ToLocalChecked());
            }
        }
    V8_UNLOCK()

    return out;

}

JS_EXPORT JSObjectRef JSObjectMakeFunction(JSContextRef ctx, JSStringRef name,
    unsigned parameterCount, const JSStringRef parameterNames[], JSStringRef body,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exceptionRef)
{
    JSObjectRef out = nullptr;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        TempException exception(exceptionRef);
        OpaqueJSString anonymous("anonymous");

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
            exception.Set(ctx, trycatch.Exception());
        }

        MaybeLocal<Value> result;

        if (!*exception) {
            result = script.ToLocalChecked()->Run(context);
            if (result.IsEmpty()) {
                exception.Set(ctx, trycatch.Exception());
            }
        }

        if (!*exception) {
            Local<Function> function = Local<Function>::Cast(result.ToLocalChecked());
            if (name) {
                function->SetName(name->Value(isolate));
            }
            out = new OpaqueJSValue(ctx, result.ToLocalChecked());
        }
    V8_UNLOCK()

    return out;
}

JS_EXPORT JSValueRef JSObjectGetPrototype(JSContextRef ctx, JSObjectRef object)
{
    JSValueRef out = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TO_REAL_GLOBAL(o);
        out = new OpaqueJSValue(ctx, o->GetPrototype());
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetPrototype(JSContextRef ctx, JSObjectRef object, JSValueRef value)
{
    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempJSValue null(JSValueMakeNull(ctx));
        if (!value) value = *null;
        TO_REAL_GLOBAL(o);
        o->SetPrototype(context, (*value)->Value());
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

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, propertyName->Value(isolate));
        if (value.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        }

        if (!*exception) {
            out = new OpaqueJSValue(ctx, value.ToLocalChecked());
        }
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
    JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exceptionRef)
{
    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);
        TempJSValue null(JSValueMakeNull(ctx));
        if (!value) value = *null;
        int v8_attr = v8::None;
        if (attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if (attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if (attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        TryCatch trycatch(isolate);

        Maybe<bool> defined = (attributes!=0) ?
            o->DefineOwnProperty(
                context,
                propertyName->Value(isolate),
                (*value)->Value(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, propertyName->Value(isolate), (*value)->Value());

        if (defined.IsNothing()) {
            exception.Set(ctx, trycatch.Exception());
        }
    V8_UNLOCK()
}

JS_EXPORT bool JSObjectDeleteProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exceptionRef)
{
    if (!propertyName) return false;

    bool v = false;
    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);

        TryCatch trycatch(isolate);

        Maybe<bool> deleted = o->Delete(context, propertyName->Value(isolate));
        if (deleted.IsNothing()) {
            exception.Set(ctx, trycatch.Exception());
        } else {
            v = deleted.FromMaybe(false);
        }
    V8_UNLOCK()

    return v;
}

JS_EXPORT JSValueRef JSObjectGetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef* exceptionRef)
{
    JSValueRef out = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);
        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->Get(context, propertyIndex);
        if (value.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        }

        if (!*exception) {
            out = new OpaqueJSValue(ctx, value.ToLocalChecked());
        }
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef value, JSValueRef* exceptionRef)
{
    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);
        TempJSValue null(JSValueMakeNull(ctx));
        if (!value) value = *null;
        TryCatch trycatch(isolate);

        Maybe<bool> defined =
            o->Set(context, propertyIndex, (*value)->Value());

        if (defined.IsNothing()) {
            exception.Set(ctx, trycatch.Exception());
        }
    V8_UNLOCK()
}

JS_EXPORT void* JSObjectGetPrivate(JSObjectRef object)
{
    void *data = nullptr;

    V8_ISOLATE_OBJ((*object)->Context(),object,isolate,context,o)
        if (!o->InternalFieldCount()) {
            if (o->GetPrototype()->IsObject())
                o = o->GetPrototype()->ToObject(context).ToLocalChecked();
        }
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

    V8_ISOLATE_OBJ((*object)->Context(),object,isolate,context,o)
        if (!o->InternalFieldCount()) {
            if (o->GetPrototype()->IsObject())
                o = o->GetPrototype()->ToObject(context).ToLocalChecked();
        }
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

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);
        Local<Value> this_ = thisObject ?
            (*thisObject)->Value() :
            Local<Value>::New(isolate,Null(isolate));

        Local<Value> *elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            if (arguments[i]) {
                elements[i] = (*arguments[i])->Value();
            } else {
                elements[i] = Local<Value>::New(isolate,Null(isolate));
            }
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, argumentCount, elements);
        if (value.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        }

        if (!*exception) {
            out = new OpaqueJSValue(ctx, value.ToLocalChecked());
        }
        delete [] elements;
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

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempException exception(exceptionRef);
        Local<Value> *elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            elements[i] = (*arguments[i])->Value();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsConstructor(context, argumentCount, elements);
        if (value.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        }

        if (!*exception) {
            out = new OpaqueJSValue(ctx, value.ToLocalChecked());
        }
        delete [] elements;
    V8_UNLOCK()

    return out;
}

JS_EXPORT JSPropertyNameArrayRef JSObjectCopyPropertyNames(JSContextRef ctx, JSObjectRef object)
{
    if (!object) return nullptr;

    JSPropertyNameArrayRef array;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        Local<Array> names = o->GetPropertyNames(context).ToLocalChecked();
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
        JSStringRetain(propertyName);
        accumulator->push_front(propertyName);
    }
}
