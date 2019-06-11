/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "FunctionTemplate.h"
#include "ObjectTemplate.h"
#include "Object.h"
#include "JSCPrivate.h"

using namespace V82JSC;
using v8::Local;
using v8::AccessorSignature;
using v8::Isolate;
using v8::MaybeLocal;
using v8::Function;
using v8::EscapableHandleScope;
using v8::Object;
using v8::HandleScope;
using v8::TryCatch;

Local<v8::Signature> v8::Signature::New(Isolate* isolate,
                                Local<FunctionTemplate> receiver)
{
    EscapableHandleScope scope(isolate);
    
    auto signature = static_cast<V82JSC::Signature *>
    (HeapAllocator::Alloc(ToIsolateImpl(isolate),
                             ToIsolateImpl(isolate)->m_signature_map));
    signature->m_template.Reset(isolate, receiver);
    
    return scope.Escape(CreateLocal<Signature>(isolate, signature));
}

Local<AccessorSignature> AccessorSignature::New(Isolate* isolate,
                                                Local<FunctionTemplate> receiver)
{
    EscapableHandleScope scope(isolate);
    
    auto signature = static_cast<V82JSC::Signature *>
    (HeapAllocator::Alloc(ToIsolateImpl(isolate),
                             ToIsolateImpl(isolate)->m_signature_map));
    signature->m_template.Reset(isolate, receiver);
    
    return scope.Escape(CreateLocal<AccessorSignature>(isolate, signature));
}

/** Get a template included in the snapshot by index. */
MaybeLocal<v8::FunctionTemplate> v8::FunctionTemplate::FromSnapshot(Isolate* isolate,
                                                            size_t index)
{
    return MaybeLocal<FunctionTemplate>();
}

Local<v8::FunctionTemplate> v8::FunctionTemplate::New(Isolate* isolate, FunctionCallback callback,
                                          Local<Value> data,
                                          Local<Signature> signature, int length,
                                          ConstructorBehavior behavior)
{
    EscapableHandleScope scope(isolate);
    
    Local<Context> context = OperatingContext(isolate);
    auto templ = static_cast<V82JSC::FunctionTemplate*>
    (HeapAllocator::Alloc(ToIsolateImpl(isolate),
                             ToIsolateImpl(isolate)->m_function_template_map));

    templ->m_name.Reset();
    templ->m_callback = callback;
    templ->m_signature.Reset(isolate, signature);
    templ->m_behavior = behavior;
    templ->m_length = length;
    templ->m_isHiddenPrototype = false;

    if (data.IsEmpty()) {
        data = Undefined(isolate);
    }
    templ->m_data = ToJSValueRef<Value>(data, isolate);
    JSValueProtect(ToContextRef(context), templ->m_data);
    templ->m_functions_map = exec(ToContextRef(context), "return new Map()", 0, 0);
    JSValueProtect(ToContextRef(context), templ->m_functions_map);
    
    return scope.Escape(CreateLocal<FunctionTemplate>(isolate, templ));
}

/**
 * Creates a function template backed/cached by a private property.
 */
Local<v8::FunctionTemplate> v8::FunctionTemplate::NewWithCache(
                                            Isolate* isolate, FunctionCallback callback,
                                            Local<v8::Private> cache_property, Local<v8::Value> data,
                                            Local<v8::Signature> signature, int length)
{
    return New(isolate, callback, data, signature, length);
}

/*
 * \code
 *   FunctionTemplate Parent  -> Parent() . prototype -> { }
 *     ^                                                  ^
 *     | Inherit(Parent)                                  | .__proto__
 *     |                                                  |
 *   FunctionTemplate Child   -> Child()  . prototype -> { }
 * \endcode
 *
 * A FunctionTemplate 'Child' inherits from 'Parent', the prototype
 * object of the Child() function has __proto__ pointing to the
 * Parent() function's prototype object. An instance of the Child
 * function has all properties on Parent's instance templates.
 */

/** Returns the unique function instance in the current execution context.*/
MaybeLocal<Function> v8::FunctionTemplate::GetFunction(Local<Context> context)
{
    return V82JSC::FunctionTemplate::GetFunction(this, context, Local<String>());
}

MaybeLocal<Function> V82JSC::FunctionTemplate::GetFunction(v8::FunctionTemplate * ft,
                                                       v8::Local<v8::Context> context,
                                                       v8::Local<v8::Name> inferred_name)
{
    auto impl = ToImpl<V82JSC::FunctionTemplate,v8::FunctionTemplate>(ft);
    auto ctximpl = ToContextImpl(context);
    JSContextRef ctx = ToContextRef(context);
    IsolateImpl* iso = ToIsolateImpl(ctximpl);
    Isolate* isolate = ToIsolate(iso);
    
    EscapableHandleScope scope(isolate);
    
    Local<v8::FunctionTemplate> thiz = CreateLocal<v8::FunctionTemplate>(isolate, impl);

    assert(impl->m_functions_map);
    auto gCtx = ToGlobalContextImpl(FindGlobalContext(context));
    assert(gCtx);
    JSObjectRef function = nullptr;
    if (gCtx->m_creation_context) {
        JSValueRef args_get[] = {
            impl->m_functions_map,
            gCtx->m_creation_context
        };
        function = (JSObjectRef) exec(ctx, "return _1.get(_2)", 2, args_get);
    }

    if (function && JSValueIsObject(gCtx->m_ctxRef, function)) {
        return scope.Escape(Value::New(ctximpl, function).As<Function>());
    }

    JSStringRef generic_function_name = JSStringCreateWithUTF8CString("generic_function");
    JSStringRef empty_body = JSStringCreateWithUTF8CString("");

    JSValueRef excp = nullptr;
    
    void * data = PersistentData<v8::Template>(isolate, thiz);
    
    JSClassDefinition function_def = kJSClassDefinitionEmpty;
    function_def.callAsFunction = Template::callAsFunctionCallback;
    function_def.className = "function_proxy";
    function_def.finalize = [](JSObjectRef object) {
        void * data = JSObjectGetPrivate(object);
        ReleasePersistentData<v8::Template>(data);
    };
    JSClassRef function_class = JSClassCreate(&function_def);
    JSObjectRef function_proxy = JSObjectMake(ctx, function_class, data);
    JSClassRelease(function_class);

    void * data2 = PersistentData<v8::Template>(isolate, thiz);

    JSClassDefinition constructor_def = kJSClassDefinitionEmpty;
    constructor_def.callAsFunction = FunctionTemplate::callAsConstructorCallback;
    constructor_def.className = "constructor_proxy";
    constructor_def.finalize = [](JSObjectRef object) {
        void * data = JSObjectGetPrivate(object);
        ReleasePersistentData<v8::Template>(data);
    };
    JSClassRef constructor_class = JSClassCreate(&constructor_def);
    JSObjectRef constructor_proxy = JSObjectMake(ctx, constructor_class, data2);
    JSClassRelease(constructor_class);
    
    static const char *proxy_function_template =
    "Object.setPrototypeOf(_1, Function.prototype); "
    "Object.setPrototypeOf(_2, Function.prototype); "
    "return (()=>{"
    "function name () { "
    "    if (new.target) { "
    "        return _2.call(this, new.target == name, new.target, ...arguments); "
    "    } else { "
    "        return _1.call(this, ...arguments); "
    "    } "
    "} "
    "if (_3) { "
    "    Object.defineProperty(name, 'length', {value: _3}); "
    "} "
    "return name "
    "})()";

    static const char *proxy_noconstructor_template =
    "Object.setPrototypeOf(_1, Function.prototype); "
    "return (()=>{"
    "var name = () => _1.call(this, ...arguments);"
    "if (_3) { "
    "    Object.defineProperty(name, 'length', {value: _3}); "
    "} "
    "return name; "
    "})()";

    auto ReplaceStringInPlace = [](std::string& subject, const std::string& search,
                              const std::string& replace) {
        size_t pos = 0;
        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    };
    
    std::string proxy_function_body = impl->m_removePrototype ? proxy_noconstructor_template : proxy_function_template;
    Local<v8::String> name_ = impl->m_name.Get(isolate);
    if (name_.IsEmpty() && !inferred_name.IsEmpty() && inferred_name->IsString()) {
        name_ = inferred_name.As<v8::String>();
    } else if (name_.IsEmpty()) {
        name_ = v8::String::NewFromUtf8(isolate, "Function", v8::NewStringType::kNormal).ToLocalChecked();
    }
    v8::String::Utf8Value str(name_);
    const char *sname = !name_.IsEmpty() ? *str : "Function";
    ReplaceStringInPlace(proxy_function_body, "name", sname);

    JSValueRef params[] = {
        function_proxy,
        constructor_proxy,
        JSValueMakeNumber(ctx, impl->m_length)
    };
    function = (JSObjectRef) exec(ToContextRef(context), proxy_function_body.c_str(), 3, params);
    
    TrackedObject::makePrivateInstance(iso, ctx, function);
    
    LocalException exception(iso);

    MaybeLocal<Object> thizo = reinterpret_cast<V82JSC::Template*>(impl)->
        InitInstance(context, function, exception);
    if (thizo.IsEmpty()) {
        return MaybeLocal<Function>();
    }
    if (!impl->m_removePrototype) {
        JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
        MaybeLocal<v8::Value> prototype;
        if (impl->m_prototype_provider.IsEmpty()) {
            Local<v8::ObjectTemplate> prototype_template = thiz->PrototypeTemplate();
            MaybeLocal<Object> pro = prototype_template->NewInstance(context);
            if (!pro.IsEmpty()) {
                prototype = pro.ToLocalChecked();
            }
        } else {
            MaybeLocal<Function> provider_function = impl->m_prototype_provider.Get(isolate)->GetFunction(context);
            if (!provider_function.IsEmpty()) {
                prototype = provider_function.ToLocalChecked()->
                Get(context, v8::String::NewFromUtf8(isolate, "prototype", v8::NewStringType::kNormal).ToLocalChecked());
            }
        }
        if (prototype.IsEmpty()) {
            return MaybeLocal<Function>();
        }
        JSValueRef prototype_property = ToJSValueRef(prototype.ToLocalChecked(), context);
        JSValueRef args[] = {
            function,
            prototype_property,
            JSValueMakeBoolean(ctx, !impl->m_readOnlyPrototype)
        };
        exec(ctx, "Object.defineProperty(_1, 'prototype', { value: _2, enumerable: false, writable: _3})", 3, args);
        JSStringRef constructor = JSStringCreateWithUTF8CString("constructor");
        JSObjectSetProperty(ToContextRef(context), (JSObjectRef)prototype_property, constructor, function, kJSPropertyAttributeDontEnum, 0);
        if (!impl->m_parent.IsEmpty()) {
            MaybeLocal<Function> parentFunc = impl->m_parent.Get(isolate)->GetFunction(context);
            if (parentFunc.IsEmpty()) {
                JSStringRelease(sprototype);
                return MaybeLocal<Function>();
            }
            JSValueRef parentFuncRef = ToJSValueRef<Function>(parentFunc.ToLocalChecked(), context);
            JSValueRef parentFuncPrototype = JSObjectGetProperty(ctx, (JSObjectRef)parentFuncRef, sprototype, 0);
            SetRealPrototype(context, (JSObjectRef)prototype_property, parentFuncPrototype, true);
        }
        JSStringRelease(sprototype);
    }

    if (gCtx->m_creation_context) {
        JSValueRef args_set[] = { impl->m_functions_map, gCtx->m_creation_context, function };
        exec(ctx, "_1.set(_2, _3)", 3, args_set);
    }

    return scope.Escape(Value::New(ctximpl, function).As<Function>());
}

/**
 * Similar to Context::NewRemoteContext, this creates an instance that
 * isn't backed by an actual object.
 *
 * The InstanceTemplate of this FunctionTemplate must have access checks with
 * handlers installed.
 */
MaybeLocal<Object> v8::FunctionTemplate::NewRemoteInstance()
{
    assert(0);
    return MaybeLocal<Object>();
}

/**
 * Set the call-handler callback for a FunctionTemplate.  This
 * callback is called whenever the function created from this
 * FunctionTemplate is called.
 */
void v8::FunctionTemplate::SetCallHandler(FunctionCallback callback,
                    Local<Value> data)
{
    auto impl =  ToImpl<V82JSC::FunctionTemplate>(this);
    IsolateImpl* iso = ToIsolateImpl(impl);
    auto ctx = ToContextRef(ToIsolate(iso));
    HandleScope scope(ToIsolate(iso));
    
    impl->m_callback = callback;
    if (!*data) {
        data = Undefined(ToIsolate(iso));
    }
    if (impl->m_data) {
        JSValueUnprotect(ctx, impl->m_data);
    }
    impl->m_data = ToJSValueRef(data, ToIsolate(iso));
    JSValueProtect(ctx, impl->m_data);
}

/** Set the predefined length property for the FunctionTemplate. */
void v8::FunctionTemplate::SetLength(int length)
{
    auto impl =  ToImpl<V82JSC::FunctionTemplate>(this);
    impl->m_length = length;
}

/** Get the InstanceTemplate. */
Local<v8::ObjectTemplate> v8::FunctionTemplate::InstanceTemplate()
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    Isolate* isolate = ToIsolate(ToIsolateImpl(impl));
    EscapableHandleScope scope(isolate);

    Local<FunctionTemplate> thiz = CreateLocal<FunctionTemplate>(isolate, impl);
    Local<ObjectTemplate> instance_template;
    if (impl->m_instance_template.IsEmpty()) {
        instance_template = ObjectTemplate::New(isolate);
        impl->m_instance_template.Reset(isolate, instance_template);
        auto instt = ToImpl<V82JSC::ObjectTemplate>(instance_template);
        instt->m_constructor_template.Reset(isolate, thiz);
        instt->m_parent.Reset(isolate, thiz);
    } else {
        instance_template = impl->m_instance_template.Get(isolate);
    }
    return scope.Escape(instance_template);
}

/**
 * Causes the function template to inherit from a parent function template.
 * This means the the function's prototype.__proto__ is set to the parent
 * function's prototype.
 **/
void v8::FunctionTemplate::Inherit(Local<FunctionTemplate> parent)
{
    auto isolate = ToIsolate<FunctionTemplate>(this);
    auto impl = ToImpl<V82JSC::FunctionTemplate,FunctionTemplate>(this);
    impl->m_parent.Reset(isolate, parent);
}

/**
 * A PrototypeTemplate is the template used to create the prototype object
 * of the function created by this template.
 */
Local<v8::ObjectTemplate> v8::FunctionTemplate::PrototypeTemplate()
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    auto isolate = ToIsolate(ToIsolateImpl(impl));
    EscapableHandleScope scope(isolate);
    
    Local<ObjectTemplate> prototype_template;
    if (impl->m_prototype_template.IsEmpty()) {
        prototype_template = ObjectTemplate::New(isolate);
        impl->m_prototype_template.Reset(isolate, prototype_template);
    } else {
        prototype_template = impl->m_prototype_template.Get(isolate);
    }
    return scope.Escape(prototype_template);
}

/**
 * A PrototypeProviderTemplate is another function template whose prototype
 * property is used for this template. This is mutually exclusive with setting
 * a prototype template indirectly by calling PrototypeTemplate() or using
 * Inherit().
 **/
void v8::FunctionTemplate::SetPrototypeProviderTemplate(Local<FunctionTemplate> prototype_provider)
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    Isolate* isolate = ToIsolate(ToIsolateImpl(impl));
    impl->m_prototype_provider.Reset(isolate, prototype_provider);
}

/**
 * Set the class name of the FunctionTemplate.  This is used for
 * printing objects created with the function created from the
 * FunctionTemplate as its constructor.
 */
void v8::FunctionTemplate::SetClassName(Local<String> name)
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    impl->m_name.Reset(ToIsolate(this), name);
}

/**
 * When set to true, no access check will be performed on the receiver of a
 * function call.  Currently defaults to true, but this is subject to change.
 */
void v8::FunctionTemplate::SetAcceptAnyReceiver(bool value)
{
    assert(0);
}

/**
 * Determines whether the __proto__ accessor ignores instances of
 * the function template.  If instances of the function template are
 * ignored, __proto__ skips all instances and instead returns the
 * next object in the prototype chain.
 *
 * Call with a value of true to make the __proto__ accessor ignore
 * instances of the function template.  Call with a value of false
 * to make the __proto__ accessor not ignore instances of the
 * function template.  By default, instances of a function template
 * are not ignored.
 */
void v8::FunctionTemplate::SetHiddenPrototype(bool value)
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    impl->m_isHiddenPrototype = value;
}

/**
 * Sets the ReadOnly flag in the attributes of the 'prototype' property
 * of functions created from this FunctionTemplate to true.
 */
void v8::FunctionTemplate::ReadOnlyPrototype()
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    impl->m_readOnlyPrototype = true;
}

/**
 * Removes the prototype property from functions created from this
 * FunctionTemplate.
 */
void v8::FunctionTemplate::RemovePrototype()
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    impl->m_removePrototype = true;
}

/**
 * Returns true if the given object is an instance of this function
 * template.
 */
bool v8::FunctionTemplate::HasInstance(Local<Value> object)
{
    auto impl = ToImpl<V82JSC::FunctionTemplate>(this);
    Isolate* isolate = ToIsolate(impl->GetIsolate());
    HandleScope scope(isolate);
    
    Local<Context> context = OperatingContext(isolate);
    if (!object->IsObject()) return false;
    
    JSValueRef o = ToJSValueRef(object, context);
    JSContextRef ctx = ToContextRef(context);
    auto wrap = V82JSC::TrackedObject::getPrivateInstance(ctx, (JSObjectRef)o);
    if (!wrap) return false;
    
    if (!wrap->m_object_template.IsEmpty()) {
        auto ot = ToImpl<V82JSC::ObjectTemplate>(wrap->m_object_template.Get(isolate));
        if (!ot->m_constructor_template.IsEmpty()) {
            return (ToImpl<V82JSC::FunctionTemplate>(ot->m_constructor_template.Get(isolate)) == impl);
        }
    }
    return false;
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

JSValueRef FunctionTemplate::callAsConstructorCallback(JSContextRef ctx,
                                                           JSObjectRef constructor_function,
                                                           JSObjectRef instance,
                                                           size_t argumentCount,
                                                           const JSValueRef *arguments,
                                                           JSValueRef *exception)
{
    IsolateImpl *isolateimpl = IsolateFromCtx(ctx);
    v8::Locker lock(ToIsolate(isolateimpl));
    
    Isolate *isolate = ToIsolate(isolateimpl);
    HandleScope scope(isolate);
    
    Local<v8::Context> context = LocalContext::New(isolate, ctx);
    auto ctximpl = ToContextImpl(context);
    auto thread = IsolateImpl::PerThreadData::Get(isolateimpl);

    assert(argumentCount>1);
    bool create_object = JSValueToBoolean(ctx, arguments[0]);
    JSObjectRef new_target = (JSObjectRef)arguments[1];
    argumentCount-=2;
    arguments+=2;

    v8::Context::Scope context_scope(context);
    
    void * private_data = JSObjectGetPrivate(constructor_function);
    Local<Template> templt = FromPersistentData<Template>(isolate, private_data);
    Local<v8::FunctionTemplate> function_template = * reinterpret_cast<Local<v8::FunctionTemplate>*>(&templt);
    auto ftempl = ToImpl<V82JSC::FunctionTemplate>(function_template);
    Local<v8::ObjectTemplate> instance_template = function_template->InstanceTemplate();
    auto otempl = ToImpl<V82JSC::ObjectTemplate>(*instance_template);
    JSObjectRef function = (JSObjectRef) ToJSValueRef(function_template->GetFunction(context).ToLocalChecked(), context);

    JSClassDefinition def = kJSClassDefinitionEmpty;
    void* data_ = nullptr;
    if (create_object) {
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        v8::String::Utf8Value str(ftempl->m_name.Get(isolate));
        if (*str) {
            const std::string& name = *str;
            def.className = name.length() ? name.c_str() : nullptr;
        }
        if (otempl->m_callback) {
            def.callAsFunction = Template::callAsFunctionCallback;
            def.callAsConstructor = Template::callAsConstructorCallback;
        } else {
            def.callAsFunction = [](JSContextRef ctx,
                                    JSObjectRef function,
                                    JSObjectRef thisObject,
                                    size_t argumentCount,
                                    const JSValueRef *arguments,
                                    JSValueRef *exception) -> JSValueRef
            {
                *exception = exec(ctx, "return new TypeError('object is not a function')", 0, 0);
                return 0;
            };
        }

        data_ = PersistentData(isolate, instance_template);
        def.finalize = [](JSObjectRef object) {
            void * data = JSObjectGetPrivate(object);
            ReleasePersistentData<Template>(data);
        };
    }
    
    TryCatch try_catch(isolate);
    
    Local<Object> thiz = otempl->NewInstance(context, create_object?0:instance,
                                             ftempl->m_isHiddenPrototype, create_object?&def:nullptr,data_).ToLocalChecked();
    if (create_object) {
        auto wrap = TrackedObject::getPrivateInstance(ctx, (JSObjectRef)ToJSValueRef(thiz, context));
        instance = (JSObjectRef)wrap->m_security;
        JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
        JSValueRef excp = 0;
        JSObjectRef prototype = (JSObjectRef) JSObjectGetProperty(ctx, function, sprototype, &excp);
        assert(excp == 0);
        SetRealPrototype(context, instance, prototype, true);
        assert(excp == 0);
    }

    Local<Object> target = Value::New(ToContextImpl(context), new_target).As<Object>();
    
    if (try_catch.HasCaught()) {
        if (exception) {
            *exception = ToJSValueRef<v8::Value>(try_catch.Exception(), context);
        }
        return 0;
    }

    thread->m_callback_depth ++;

    Local<v8::Value> data = Value::New(ToContextImpl(context), ftempl->m_data);
    typedef v8::internal::Heap::RootListIndex R;
    v8::internal::Object *the_hole = isolateimpl->ii.heap()->root(R::kTheHoleValueRootIndex);

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 0;
        O(isolateimpl),                                      // kIsolateIndex = 1;
        the_hole,                                            // kReturnValueDefaultValueIndex = 2;
        the_hole,                                            // kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        * reinterpret_cast<v8::internal::Object**>(*target)  // kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<v8::Value> arg = Value::New(ctximpl, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    FunctionCallback info(implicit, values, (int) argumentCount);
    
    thread->m_scheduled_exception = the_hole;

    if (ftempl->m_callback) {
        ftempl->m_callback(info);
    }
    
    if (try_catch.HasCaught()) {
        *exception = ToJSValueRef(try_catch.Exception(), context);
    } else if (thread->m_scheduled_exception != the_hole) {
        v8::internal::Object * excep = thread->m_scheduled_exception;
        *exception = ToJSValueRef_<v8::Value>(excep, context);
        thread->m_scheduled_exception = the_hole;
    }

    -- thread->m_callback_depth;
    
    if (implicit[3] == the_hole) {
        return ToJSValueRef<Object>(thiz, context);
    }

    Local<v8::Value> ret = info.GetReturnValue().Get();
    if (!ret->IsObject()) {
        return ToJSValueRef<Object>(thiz, context);
    }
    return (JSObjectRef) ToJSValueRef<v8::Value>(ret, isolate);
}

