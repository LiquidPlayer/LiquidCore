/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSC/Macros.h"
#include "JSC/OpaqueJSContext.h"
#include "JSC/OpaqueJSContextGroup.h"
#include "JSC/OpaqueJSValue.h"
#include "JSC/OpaqueJSClass.h"
#include "JSC/OpaqueJSString.h"
#include "JSC/ObjectData.h"
#include "JSC/TempException.h"

JSClassRef OpaqueJSClass::New(const JSClassDefinition *definition)
{
    return new OpaqueJSClass(definition);
}

OpaqueJSClass::OpaqueJSClass(const JSClassDefinition *definition)
{
    m_definition = new JSClassDefinition;
    memcpy(const_cast<JSClassDefinition*>(m_definition), definition, sizeof(JSClassDefinition));

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
        const char *str = ObjectData::Get(info.Data())->Name();

        JSValueRef arguments[info.Length()];
        for (int i=0; i<info.Length(); i++) {
            arguments[i] = OpaqueJSValue::New(ctxRef_, info[i]);
        }
        TempJSValue function(ctxRef_, ObjectData::Get(info.Data())->Func());

        TempJSValue thisObject(ctxRef_, info.This());

        TempException exception(nullptr);
        TempJSValue value;

        while (definition && !*exception && !*value) {
            for (int i=0; !*value && !*exception &&
                definition->staticFunctions && definition->staticFunctions[i].name;
                i++) {

                if (!strcmp(definition->staticFunctions[i].name, str) &&
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
            isolate->ThrowException((*exception)->L());
        }

        if (*value) {
            info.GetReturnValue().Set((*value)->L());
        }

    V8_UNLOCK()
}

void OpaqueJSClass::ConvertFunctionCallHandler(const FunctionCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        String::Utf8Value const str(isolate, info[0]);

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
            isolate->ThrowException((*exception)->L());
        }

        if (*value) {
            info.GetReturnValue().Set((*value)->L());
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

        if (ObjectData::Get(info.Data())->Class() && info[0]->IsObject()) {

            JSClassRef ctor = ObjectData::Get(info.Data())->Class();
            if (info[0].As<Object>()->InternalFieldCount() > INSTANCE_OBJECT_CLASS) {
                JSClassRef inst =
                    (JSClassRef) info[0].As<Object>()->GetAlignedPointerFromInternalField(INSTANCE_OBJECT_CLASS);
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
            isolate->ThrowException((*exception)->L());
        }

        if (*value) {
            info.GetReturnValue().Set((*value)->L());
        }
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertyQuerier(Local< Name > property,
    const PropertyCallbackInfo< Integer > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempJSValue thisObject(ctxRef_, info.This());

        String::Utf8Value const str(isolate, property);
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

void OpaqueJSClass::ProtoPropertyQuerier(Local< Name > property,
    const PropertyCallbackInfo< Integer > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        String::Utf8Value const str(isolate, property);

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

void OpaqueJSClass::NamedPropertyGetter(Local< Name > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);
        TempJSValue thisObject(ctxRef_, info.This());
        TempJSValue value;

        String::Utf8Value const str(isolate, property);
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
                        error = String::Concat(error, property.As<String>());
                        exception.Set(ctxRef_, Exception::Error(error));
                    }
                }
                definition = definition->parentClass ?definition->parentClass->m_definition:nullptr;
            }
        }

        // ... then its prototype chain.

        if (*exception) {
            isolate->ThrowException((*exception)->L());
        }

        if (*value) {
            info.GetReturnValue().Set((*value)->L());
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

void OpaqueJSClass::ProtoPropertyGetter(Local< Name > property,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);
        TempJSValue thisObject(ctxRef_, info.This());
        TempJSValue value;

        String::Utf8Value const str(isolate, property);
        OpaqueJSString string(*str);

        while (definition && !*value && !*exception) {
            // Check static functions
            for (int i=0; !*value && !*exception &&
                definition->staticFunctions && definition->staticFunctions[i].name;
                i++) {

                if (!strcmp(definition->staticFunctions[i].name, *str) &&
                    definition->staticFunctions[i].callAsFunction) {

                    Local<Value> data = ObjectData::New(definition, ctxRef_);
                    ObjectData::Get(data)->SetName(property);

                    Local<FunctionTemplate> ftempl =
                        FunctionTemplate::New(isolate, StaticFunctionCallHandler, data);
                    Local<Function> func = ftempl->GetFunction();
                    ObjectData::Get(data)->SetFunc(func);
                    value.Set(ctxRef_, func);
                }
            }

            // then its parent class chain (which includes the default object class)
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        if (*exception) {
            isolate->ThrowException((*exception)->L());
        }

        if (*value) {
            info.GetReturnValue().Set((*value)->L());
        }
    V8_UNLOCK()
}

void OpaqueJSClass::NamedPropertySetter(Local< Name > property, Local< Value > value,
    const PropertyCallbackInfo< Value > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);
        TempJSValue thisObject(ctxRef_, info.This());
        TempJSValue valueRef(ctxRef_,value);

        String::Utf8Value const str(isolate, property);
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
            isolate->ThrowException((*exception)->L());
            value = (*exception)->L();
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

void OpaqueJSClass::NamedPropertyDeleter(Local< Name > property,
    const PropertyCallbackInfo< Boolean > &info)
{
    V8_ISOLATE_CALLBACK(info,isolate,context,definition)
        TempException exception(nullptr);

        TempJSValue thisObject(ctxRef_, info.This());

        String::Utf8Value const str(isolate, property);
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
            isolate->ThrowException((*exception)->L());
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
        while (!accumulator.empty()) {
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
        while (!accumulator.empty()) {
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
        while (!accumulator.empty()) {
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
            arguments[i] = OpaqueJSValue::New(ctxRef_, info[i]);
        }
        TempJSValue function(ctxRef_, ObjectData::Get(info.Data())->Func());
        TempJSValue thisObject(ctxRef_, info.This());

        TempJSValue value;
        while (definition && !*exception && !*value) {
            if (info.IsConstructCall() && definition->callAsConstructor) {
                value.Set(definition->callAsConstructor(
                    ctxRef_,
                    const_cast<JSObjectRef>(*function),
                    (size_t) info.Length(),
                    arguments,
                    &exception));
                if (!*value || !(*value)->L()->IsObject()) {
                    value.Reset();
                    Local<String> error = String::NewFromUtf8(isolate, "Bad constructor");
                    exception.Set(ctxRef_, Exception::Error(error));
                }
            } else if (info.IsConstructCall() && ObjectData::Get(info.Data())->Class()) {
                value.Set(
                    JSObjectMake(ctxRef_, ObjectData::Get(info.Data())->Class(), nullptr));
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
            isolate->ThrowException((*exception)->L());
        }

        if (*value) {
            info.GetReturnValue().Set((*value)->L());
        }
    V8_UNLOCK()
}

void OpaqueJSClass::Finalize(const WeakCallbackInfo<UniquePersistent<Object>>& info)
{
    auto objRef = reinterpret_cast<JSObjectRef>(info.GetInternalField(INSTANCE_OBJECT_JSOBJECT));
    /* Note: A weak callback will only retain the first two internal fields
     * But the first one is reserved.  So we will have nulled out the second one in the
     * OpaqueJSValue destructor.
     * I am intentionally not using the macro here to ensure that we always
     * read position one, even if the indices move later.
     */
    if ((info.GetInternalField(1) != nullptr) && objRef && !objRef->HasFinalized()) {
        objRef->SetFinalized();
        auto definition = objRef->Definition();
        while (definition) {
            if (definition->finalize) {
                definition->finalize(objRef);
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }
    }
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

JSGlobalContextRef OpaqueJSClass::NewContext(JSContextGroupRef group) {
    JSGlobalContextRef ctx;

    V8_ISOLATE(const_cast<OpaqueJSContextGroup*>(group)->ContextGroup::shared_from_this(), isolate)
        Local<Value> data = ObjectData::New(m_definition);

        Local<ObjectTemplate> object = ObjectTemplate::New(isolate);

        NamedPropertyHandlerConfiguration config(
                NamedPropertyGetter,
                NamedPropertySetter,
                NamedPropertyQuerier,
                NamedPropertyDeleter,
                NamedPropertyEnumerator,
                data,
                PropertyHandlerFlags::kOnlyInterceptStrings);

        object->SetHandler(config);

        object->SetIndexedPropertyHandler(
            IndexedPropertyGetter,
            IndexedPropertySetter,
            IndexedPropertyQuerier,
            IndexedPropertyDeleter,
            IndexedPropertyEnumerator,
            data);

        if (IsFunction() || IsConstructor()) {
            object->SetCallAsFunctionHandler(CallAsFunction, data);
        }

        object->SetInternalFieldCount(INSTANCE_OBJECT_FIELDS);

        Local<Context> context = Context::New(isolate, nullptr, object);
        {
            Context::Scope context_scope_(context);
            Local<Object> global =
                context->Global()->GetPrototype()->ToObject(context).ToLocalChecked();
            ctx = OpaqueJSContext::New(
                JSContext::New(
                    const_cast<OpaqueJSContextGroup*>(group)->ContextGroup::shared_from_this(), context
                )
            );
            TempJSValue value(InitInstance(ctx, global, data, nullptr));
        }
    V8_UNLOCK()

    return ctx;
}

void OpaqueJSClass::NewTemplate(Local<Context> context, Local<Value> *data,
    Local<ObjectTemplate> *object)
{
    Isolate *isolate = context->GetIsolate();

    Context::Scope context_scope_(context);

    *object = ObjectTemplate::New(isolate);

    // Create data object
    *data = ObjectData::New(m_definition);

    NamedPropertyHandlerConfiguration config(
            NamedPropertyGetter,
            NamedPropertySetter,
            NamedPropertyQuerier,
            NamedPropertyDeleter,
            NamedPropertyEnumerator,
            *data,
            PropertyHandlerFlags::kOnlyInterceptStrings);

    (*object)->SetHandler(config);

    (*object)->SetIndexedPropertyHandler(
        IndexedPropertyGetter,
        IndexedPropertySetter,
        IndexedPropertyQuerier,
        IndexedPropertyDeleter,
        IndexedPropertyEnumerator,
        *data);

    if (IsFunction() || IsConstructor()) {
        (*object)->SetCallAsFunctionHandler(CallAsFunction, *data);
    }

    (*object)->SetInternalFieldCount(INSTANCE_OBJECT_FIELDS);
}

JSObjectRef OpaqueJSClass::InitInstance(JSContextRef ctx, Local<Object> instance,
    Local<Value> data, void *privateData)
{
    JSObjectRef retObj;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        ObjectData::Get(data)->SetContext(ctx);

        if (IsFunction() || IsConstructor()) {
            ObjectData::Get(data)->SetFunc(instance);
        }
        if (IsFunction()) {
            retObj = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, instance.As<Function>(), m_definition));
        } else {
            retObj = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, instance, m_definition));
        }

        auto weak = new UniquePersistent<Object>(isolate, instance);
        weak->SetWeak<UniquePersistent<Object>>(
            weak,
            Finalize,
            v8::WeakCallbackType::kInternalFields);

        instance->SetAlignedPointerInInternalField(INSTANCE_OBJECT_CLASS,this);
        retain();
        instance->SetAlignedPointerInInternalField(INSTANCE_OBJECT_JSOBJECT,(void*)retObj);
        retObj->SetPrivateData(privateData);

        // Set up a prototype object to handle static functions
        Local<ObjectTemplate> protoTemplate = ObjectTemplate::New(isolate);
        NamedPropertyHandlerConfiguration config(
                ProtoPropertyGetter,
                nullptr,
                ProtoPropertyQuerier,
                nullptr,
                ProtoPropertyEnumerator,
                data,
                PropertyHandlerFlags::kOnlyInterceptStrings);
        protoTemplate->SetHandler(config);
        Local<Object> prototype = protoTemplate->NewInstance(context).ToLocalChecked();
        instance->SetPrototype(context, prototype);

        // Set className
        const JSClassDefinition *definition = m_definition;
        while (true) {
            const char* sClassName = definition ? definition->className : "CallbackObject";
            if (sClassName) {
                Local<String> className = String::NewFromUtf8(isolate, sClassName);
                Local<Object> Symbol =
                    context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject(context).ToLocalChecked();
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
                    context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject(context).ToLocalChecked();
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
                    context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject(context).ToLocalChecked();
                Local<Value> hasInstance = Symbol->Get(String::NewFromUtf8(isolate, "hasInstance"));
                prototype->Set(context, hasInstance, function);
                break;
            }
            definition = definition->parentClass ? definition->parentClass->m_definition : nullptr;
        }

        // Find the greatest ancestor
        for (definition = m_definition; definition && definition->parentClass;
            definition = definition->parentClass->m_definition);

        // Walk backwards and call 'initialize' on each
        while (definition) {
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
