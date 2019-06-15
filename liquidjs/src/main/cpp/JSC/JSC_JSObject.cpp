/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSC/Macros.h"
#include "JSC/JSC.h"
#include "JSC/ObjectData.h"
#include "JSC/TempException.h"

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
    return OpaqueJSClass::New(definition);
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
            Local<Value> payload;
            Local<ObjectTemplate> templ;
            jsClass->NewTemplate(context, &payload, &templ);
            Local<Object> instance = templ->NewInstance(context).ToLocalChecked();
            value = jsClass->InitInstance(ctx, instance, payload, data);
        } else {
            value = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, Object::New(isolate)));
        }
    V8_UNLOCK()

    return value;
}

static JSObjectRef SetUpFunction(JSContextRef ctx, JSStringRef name, JSClassDefinition *definition,
    JSClassRef jsClass, bool isConstructor)
{
    JSObjectRef obj;
    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<Value> data = ObjectData::New(definition, ctx, jsClass);

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
                context->Global()->Get(String::NewFromUtf8(isolate, "Symbol"))->ToObject(context).ToLocalChecked();
            Local<Value> hasInstance = Symbol->Get(String::NewFromUtf8(isolate, "hasInstance"));
            Local<Object> prototype = Object::New(isolate);
            prototype->Set(context, hasInstance, function);
            func->SetPrototype(context, prototype);
        }

        ObjectData::Get(data)->SetFunc(func);

        obj = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, func));
    V8_UNLOCK()

    return obj;
}

JS_EXPORT JSObjectRef JSObjectMakeFunctionWithCallback(JSContextRef ctx, JSStringRef name,
    JSObjectCallAsFunctionCallback callAsFunction)
{
    auto definition = new JSClassDefinition;
    memset(definition, 0, sizeof(JSClassDefinition));
    definition->callAsFunction = callAsFunction;

    return SetUpFunction(ctx, name, definition, nullptr, false);
}

JS_EXPORT JSObjectRef JSObjectMakeConstructor(JSContextRef ctx, JSClassRef jsClass,
    JSObjectCallAsConstructorCallback callAsConstructor)
{
    auto definition = new JSClassDefinition;
    memset(definition, 0, sizeof(JSClassDefinition));
    definition->callAsConstructor = callAsConstructor;

    return SetUpFunction(ctx, nullptr, definition, jsClass, true);
}

JS_EXPORT JSObjectRef JSObjectMakeArray(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* )
{
    JSObjectRef object;

    V8_ISOLATE_CTX(CTX(ctx),isolate,context)
        Local<Array> array = Array::New(isolate, (int)argumentCount);
        for(size_t i=0; i<argumentCount; i++) {
            array->Set(context, (int)i, arguments[i]->L());
        }
        object = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, array));
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
                context->Global()->Get(String::NewFromUtf8(isolate, "Date"))->ToObject(context).ToLocalChecked();
            Local<Function> now = DATE->Get(String::NewFromUtf8(isolate, "now")).As<Function>();

            date = Date::New(isolate,
                now->Call(Local<Value>::New(isolate,Null(isolate)), 0, nullptr)
                    ->ToNumber(context).ToLocalChecked()->Value());
        } else {
            TryCatch trycatch(isolate);

            MaybeLocal<Number> number = arguments[0]->L()->ToNumber(context);
            double epoch;
            if (!number.IsEmpty()) {
                epoch = number.ToLocalChecked()->Value();
            } else {
                exception.Set(ctx, trycatch.Exception());
                epoch = 0.0;
            }

            date = Date::New(isolate, epoch);
        }

        out = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, date));
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
            MaybeLocal<String> maybe = arguments[0]->L()->ToString(context);
            if (!maybe.IsEmpty()) {
                str = maybe.ToLocalChecked();
            } else {
                exception.Set(ctx, trycatch.Exception());
            }
        }

        out = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, Exception::Error(str)));
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
            MaybeLocal<String> maybe = arguments[0]->L()->ToString(context);
            if (!maybe.IsEmpty()) {
                pattern = maybe.ToLocalChecked();
            } else {
                exception.Set(ctx, trycatch.Exception());
            }
        }

        if (!*exception && argumentCount > 1) {
            TryCatch trycatch(isolate);
            MaybeLocal<String> maybe = arguments[1]->L()->ToString(context);
            if (!maybe.IsEmpty()) {
                flags_ = maybe.ToLocalChecked();
            } else {
                exception.Set(ctx, trycatch.Exception());
            }
        }

        if (!*exception) {
            String::Utf8Value const str(isolate, flags_);
            unsigned flags = RegExp::Flags::kNone;
            for (size_t i=0; i<strlen(*str); i++) {
                switch ((*str)[i]) {
                    case 'g': flags |= RegExp::Flags::kGlobal;     break;
                    case 'i': flags |= RegExp::Flags::kIgnoreCase; break;
                    case 'm': flags |= RegExp::Flags::kMultiline;  break;
                    default: break;
                }
            }

            TryCatch trycatch(isolate);

            MaybeLocal<RegExp> regexp = RegExp::New(context, pattern, (RegExp::Flags)flags);
            if (regexp.IsEmpty()) {
                exception.Set(ctx, trycatch.Exception());
            } else {
                out = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, regexp.ToLocalChecked()));
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
            out = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, result.ToLocalChecked()));
        }
    V8_UNLOCK()

    return out;
}

JS_EXPORT JSValueRef JSObjectGetPrototype(JSContextRef ctx, JSObjectRef object)
{
    JSValueRef out = nullptr;

    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TO_REAL_GLOBAL(o);
        out = OpaqueJSValue::New(ctx, o->GetPrototype());
    V8_UNLOCK()

    return out;
}

JS_EXPORT void JSObjectSetPrototype(JSContextRef ctx, JSObjectRef object, JSValueRef value)
{
    V8_ISOLATE_OBJ(CTX(ctx),object,isolate,context,o)
        TempJSValue null(JSValueMakeNull(ctx));
        if (!value) value = *null;
        TO_REAL_GLOBAL(o);
        o->SetPrototype(context, value->L());
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
            out = OpaqueJSValue::New(ctx, value.ToLocalChecked());
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
        unsigned v8_attr = v8::None;
        if (attributes & kJSPropertyAttributeReadOnly) v8_attr |= v8::ReadOnly;
        if (attributes & kJSPropertyAttributeDontEnum) v8_attr |= v8::DontEnum;
        if (attributes & kJSPropertyAttributeDontDelete) v8_attr |= v8::DontDelete;

        TryCatch trycatch(isolate);

        Maybe<bool> defined = (attributes!=0) ?
            o->DefineOwnProperty(
                context,
                propertyName->Value(isolate),
                value->L(),
                static_cast<PropertyAttribute>(v8_attr))
            :
            o->Set(context, propertyName->Value(isolate), value->L());

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
            out = OpaqueJSValue::New(ctx, value.ToLocalChecked());
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
            o->Set(context, propertyIndex, value->L());

        if (defined.IsNothing()) {
            exception.Set(ctx, trycatch.Exception());
        }
    V8_UNLOCK()
}

JS_EXPORT void* JSObjectGetPrivate(JSObjectRef object)
{
    return object->GetPrivateData();
}

JS_EXPORT bool JSObjectSetPrivate(JSObjectRef object, void* data)
{
    return object->SetPrivateData(data);
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
            thisObject->L() :
            Local<Value>::New(isolate,Null(isolate));

        auto elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            if (arguments[i]) {
                elements[i] = arguments[i]->L();
            } else {
                elements[i] = Local<Value>::New(isolate,Null(isolate));
            }
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsFunction(context, this_, (int)argumentCount, elements);
        if (value.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        }

        if (!*exception) {
            out = OpaqueJSValue::New(ctx, value.ToLocalChecked());
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
        auto elements = new Local<Value>[argumentCount];
        for (size_t i=0; i<argumentCount; i++) {
            elements[i] = arguments[i]->L();
        }

        TryCatch trycatch(isolate);

        MaybeLocal<Value> value = o->CallAsConstructor(context, (int)argumentCount, elements);
        if (value.IsEmpty()) {
            exception.Set(ctx, trycatch.Exception());
        }

        if (!*exception) {
            out = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, value.ToLocalChecked()));
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
        array = const_cast<JSObjectRef>(OpaqueJSValue::New(ctx, names));
        array->Retain();
    V8_UNLOCK()

    return array;
}

JS_EXPORT JSPropertyNameArrayRef JSPropertyNameArrayRetain(JSPropertyNameArrayRef array)
{
    if (!array) return nullptr;

    V8_ISOLATE_CTX(CTX(array->Context()),isolate,context)
        array->Retain();
    V8_UNLOCK()

    return array;
}

JS_EXPORT void JSPropertyNameArrayRelease(JSPropertyNameArrayRef array)
{
    if (!array) return;

    V8_ISOLATE_CTX(CTX(array->Context()),isolate,context)
        array->Release();
    V8_UNLOCK()
}

JS_EXPORT size_t JSPropertyNameArrayGetCount(JSPropertyNameArrayRef array)
{
    if (!array) return 0;

    size_t size;

    V8_ISOLATE_CTX(CTX(array->Context()),isolate,context)
        size = array->L().As<Array>()->Length();
    V8_UNLOCK()

    return size;
}

JS_EXPORT JSStringRef JSPropertyNameArrayGetNameAtIndex(JSPropertyNameArrayRef array, size_t index)
{
    if (!array) return nullptr;

    JSStringRef ret = nullptr;

    V8_ISOLATE_CTX(CTX(array->Context()),isolate,context)
        if (index < array->L().As<Array>()->Length()) {
            Local<Value> element = array->L().As<Array>()->Get(context, index).ToLocalChecked();
            String::Utf8Value const str(isolate, element->ToString(context).ToLocalChecked());
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
