//
//  FunctionTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"
#include "JSObjectRefPrivate.h"

using namespace v8;

#define H V82JSC_HeapObject

Local<Signature> Signature::New(Isolate* isolate,
                                Local<FunctionTemplate> receiver)
{
    SignatureImpl * signature = static_cast<SignatureImpl *>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate),
                             V82JSC::ToIsolateImpl(isolate)->m_signature_map));
    signature->m_template.Reset(isolate, receiver);
    
    return V82JSC::CreateLocal<Signature>(isolate, signature);
}

Local<AccessorSignature> AccessorSignature::New(Isolate* isolate,
                                                Local<FunctionTemplate> receiver)
{
    SignatureImpl * signature = static_cast<SignatureImpl *>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate),
                             V82JSC::ToIsolateImpl(isolate)->m_signature_map));
    signature->m_template.Reset(isolate, receiver);
    
    return V82JSC::CreateLocal<AccessorSignature>(isolate, signature);
}

/** Get a template included in the snapshot by index. */
MaybeLocal<FunctionTemplate> FunctionTemplate::FromSnapshot(Isolate* isolate,
                                                            size_t index)
{
    return MaybeLocal<FunctionTemplate>();
}

Local<FunctionTemplate> FunctionTemplate::New(Isolate* isolate, FunctionCallback callback,
                                          Local<Value> data,
                                          Local<Signature> signature, int length,
                                          ConstructorBehavior behavior)
{
    Local<Context> context = V82JSC::OperatingContext(isolate);
    FunctionTemplateImpl *templ = static_cast<FunctionTemplateImpl*>
    (H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate),
                             V82JSC::ToIsolateImpl(isolate)->m_function_template_map));

    templ->m_name.Reset();
    templ->m_callback = callback;
    templ->m_signature.Reset(isolate, signature);
    templ->m_behavior = behavior;
    templ->m_length = length;
    templ->m_isHiddenPrototype = false;

    if (data.IsEmpty()) {
        data = Undefined(isolate);
    }
    templ->m_data = V82JSC::ToJSValueRef<Value>(data, isolate);
    JSValueProtect(V82JSC::ToContextRef(context), templ->m_data);
    templ->m_functions_array = JSObjectMakeArray(V82JSC::ToContextRef(context), 0, nullptr, 0);
    JSValueProtect(V82JSC::ToContextRef(context), templ->m_functions_array);
    
    return V82JSC::CreateLocal<FunctionTemplate>(isolate, templ);
}

/**
 * Creates a function template backed/cached by a private property.
 */
Local<FunctionTemplate> FunctionTemplate::NewWithCache(
                                            Isolate* isolate, FunctionCallback callback,
                                            Local<Private> cache_property, Local<Value> data,
                                            Local<Signature> signature, int length)
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
MaybeLocal<Function> FunctionTemplate::GetFunction(Local<Context> context)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    const ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctximpl);
    Isolate* isolate = V82JSC::ToIsolate(iso);
    
    EscapableHandleScope scope(isolate);

    Local<FunctionTemplate> thiz = V82JSC::CreateLocal<FunctionTemplate>(isolate, impl);

    assert(impl->m_functions_array);
    int length = static_cast<int>(JSValueToNumber(ctx, V82JSC::exec(ctx, "return _1.length",
                                                                    1, &impl->m_functions_array), 0));
    JSObjectRef function = 0;
    char index[32];
    for (auto i=0; i < length; ++i) {
        sprintf(index, "%d", i);
        JSStringRef s = JSStringCreateWithUTF8CString(index);
        JSValueRef maybe_function = JSObjectGetProperty(ctx, impl->m_functions_array, s, 0);
        JSStringRelease(s);
        if (JSObjectGetGlobalContext((JSObjectRef)maybe_function) == JSContextGetGlobalContext(ctx)) {
            function = (JSObjectRef)maybe_function;
            break;
        }
    }
    if (function) {
        return scope.Escape(ValueImpl::New(ctximpl, function).As<Function>());
    }

    JSStringRef generic_function_name = JSStringCreateWithUTF8CString("generic_function");
    JSStringRef empty_body = JSStringCreateWithUTF8CString("");

    JSValueRef excp = nullptr;
    JSObjectRef generic_function = JSObjectMakeFunction(V82JSC::ToContextRef(context),
                                    generic_function_name, 0, 0, empty_body, 0, 0, &excp);
    assert(excp == nullptr);
    JSStringRelease(generic_function_name);
    JSStringRelease(empty_body);
    JSValueRef generic_function_prototype = V82JSC::exec(V82JSC::ToContextRef(context),
                                                         "return Object.getPrototypeOf(_1)", 1,
                                                         &generic_function);

    void * data = V82JSC::PersistentData<Template>(isolate, thiz);
    
    JSClassDefinition function_def = kJSClassDefinitionEmpty;
    function_def.callAsFunction = TemplateImpl::callAsFunctionCallback;
    function_def.className = "function_proxy";
    function_def.finalize = [](JSObjectRef object) {
        void * data = JSObjectGetPrivate(object);
        V82JSC::ReleasePersistentData<Template>(data);
    };
    JSClassRef function_class = JSClassCreate(&function_def);
    JSObjectRef function_proxy = JSObjectMake(ctx, function_class, data);
    V82JSC::SetRealPrototype(context, function_proxy, generic_function_prototype);
    JSClassRelease(function_class);

    void * data2 = V82JSC::PersistentData<Template>(isolate, thiz);

    JSClassDefinition constructor_def = kJSClassDefinitionEmpty;
    constructor_def.callAsFunction = FunctionTemplateImpl::callAsConstructorCallback;
    constructor_def.className = "constructor_proxy";
    constructor_def.finalize = [](JSObjectRef object) {
        void * data = JSObjectGetPrivate(object);
        V82JSC::ReleasePersistentData<Template>(data);
    };
    JSClassRef constructor_class = JSClassCreate(&constructor_def);
    JSObjectRef constructor_proxy = JSObjectMake(ctx, constructor_class, data2);
    V82JSC::SetRealPrototype(context, constructor_proxy, generic_function_prototype);
    JSClassRelease(constructor_class);
    
    static const char *proxy_function_template =
    "function name () { "
    "    if (new.target) { "
    "        return name_ctor.call(this, new.target == name, ...arguments); "
    "    } else { "
    "        return name_func.call(this, ...arguments); "
    "    } "
    "}; "
    "if (name_length) { "
    "    Object.defineProperty(name, 'length', {value: name_length}); "
    "} "
    "return name; ";

    static const char *proxy_noconstructor_template =
    "var name = () => name_func.call(this, ...arguments);"
    "if (name_length) { "
    "    Object.defineProperty(name, 'length', {value: name_length}); "
    "} "
    "return name; ";

    auto ReplaceStringInPlace = [](std::string& subject, const std::string& search,
                              const std::string& replace) {
        size_t pos = 0;
        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    };
    
    std::string proxy_function_body = impl->m_removePrototype ? proxy_noconstructor_template : proxy_function_template;
    Local<String> name_ = impl->m_name.Get(isolate);
    if (name_.IsEmpty()) name_ = v8::String::NewFromUtf8(isolate, "Function", NewStringType::kNormal).ToLocalChecked();
    String::Utf8Value str(name_);
    const char *sname = !name_.IsEmpty() ? *str : "Function";
    ReplaceStringInPlace(proxy_function_body, "name", sname);
    
    JSStringRef name = JSStringCreateWithUTF8CString("proxy_function");
    JSStringRef body = JSStringCreateWithUTF8CString(proxy_function_body.c_str());
    JSStringRef paramNames[] = {
        JSStringCreateWithUTF8CString(std::string(std::string(sname) + "_func").c_str()),
        JSStringCreateWithUTF8CString(std::string(std::string(sname) + "_ctor").c_str()),
        JSStringCreateWithUTF8CString(std::string(std::string(sname) + "_length").c_str()),
    };
    JSValueRef exp = nullptr;
    JSObjectRef get_proxy = JSObjectMakeFunction(V82JSC::ToContextRef(context),
                                                 name,
                                                 sizeof paramNames / sizeof (JSStringRef),
                                                 paramNames,
                                                 body, 0, 0, &exp);
    assert(exp==nullptr);
    JSStringRelease(name);
    JSStringRelease(body);
    for (int i=0; i < sizeof paramNames / sizeof (JSStringRef); i++) {
        JSStringRelease(paramNames[i]);
    }
    JSValueRef params[] = {
        function_proxy,
        constructor_proxy,
        JSValueMakeNumber(ctx, impl->m_length)
    };
    function = (JSObjectRef) JSObjectCallAsFunction(V82JSC::ToContextRef(context),
                                                    get_proxy, 0, sizeof params / sizeof (JSValueRef), params, &exp);
    assert(exp==nullptr);
    
    makePrivateInstance(iso, ctx, function);
    
    LocalException exception(iso);

    MaybeLocal<Object> thizo = reinterpret_cast<TemplateImpl*>(impl)->
        InitInstance(context, function, exception);
    if (thizo.IsEmpty()) {
        return MaybeLocal<Function>();
    }
    if (!impl->m_removePrototype) {
        JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
        MaybeLocal<Value> prototype;
        if (impl->m_prototype_provider.IsEmpty()) {
            Local<ObjectTemplate> prototype_template = thiz->PrototypeTemplate();
            MaybeLocal<Object> pro = prototype_template->NewInstance(context);
            if (!pro.IsEmpty()) {
                prototype = pro.ToLocalChecked();
            }
        } else {
            MaybeLocal<Function> provider_function = impl->m_prototype_provider.Get(isolate)->GetFunction(context);
            if (!provider_function.IsEmpty()) {
                prototype = provider_function.ToLocalChecked()->
                    Get(context, String::NewFromUtf8(isolate, "prototype", NewStringType::kNormal).ToLocalChecked());
            }
        }
        if (prototype.IsEmpty()) {
            return MaybeLocal<Function>();
        }
        JSValueRef prototype_property = V82JSC::ToJSValueRef(prototype.ToLocalChecked(), context);
        JSValueRef args[] = {
            function,
            prototype_property,
            JSValueMakeBoolean(ctx, !impl->m_readOnlyPrototype)
        };
        V82JSC::exec(ctx, "Object.defineProperty(_1, 'prototype', { value: _2, enumerable: false, writable: _3})", 3, args);
        JSStringRef constructor = JSStringCreateWithUTF8CString("constructor");
        JSObjectSetProperty(V82JSC::ToContextRef(context), (JSObjectRef)prototype_property, constructor, function, kJSPropertyAttributeDontEnum, 0);
        if (!impl->m_parent.IsEmpty()) {
            MaybeLocal<Function> parentFunc = impl->m_parent.Get(isolate)->GetFunction(context);
            if (parentFunc.IsEmpty()) {
                JSStringRelease(sprototype);
                return MaybeLocal<Function>();
            }
            JSValueRef parentFuncRef = V82JSC::ToJSValueRef<Function>(parentFunc.ToLocalChecked(), context);
            JSValueRef parentFuncPrototype = JSObjectGetProperty(ctx, (JSObjectRef)parentFuncRef, sprototype, 0);
            V82JSC::SetRealPrototype(context, (JSObjectRef)prototype_property, parentFuncPrototype);
        }
        JSStringRelease(sprototype);
    }

    JSValueRef args[] = { impl->m_functions_array, function };
    V82JSC::exec(ctx, "_1.push(_2)", 2, args);

    return scope.Escape(ValueImpl::New(ctximpl, function).As<Function>());
}

/**
 * Similar to Context::NewRemoteContext, this creates an instance that
 * isn't backed by an actual object.
 *
 * The InstanceTemplate of this FunctionTemplate must have access checks with
 * handlers installed.
 */
MaybeLocal<Object> FunctionTemplate::NewRemoteInstance()
{
    assert(0);
    return MaybeLocal<Object>();
}

/**
 * Set the call-handler callback for a FunctionTemplate.  This
 * callback is called whenever the function created from this
 * FunctionTemplate is called.
 */
void FunctionTemplate::SetCallHandler(FunctionCallback callback,
                    Local<Value> data)
{
    FunctionTemplateImpl *impl =  V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(impl);
    impl->m_callback = callback;
    if (!*data) {
        data = Undefined(V82JSC::ToIsolate(iso));
    }
    impl->m_data = V82JSC::ToJSValueRef(data, V82JSC::ToIsolate(iso));
}

/** Set the predefined length property for the FunctionTemplate. */
void FunctionTemplate::SetLength(int length)
{
    FunctionTemplateImpl *impl =  V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    impl->m_length = length;
}

/** Get the InstanceTemplate. */
Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
    FunctionTemplateImpl *impl =  V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(impl));
    Local<FunctionTemplate> thiz = V82JSC::CreateLocal<FunctionTemplate>(isolate, impl);
    Local<ObjectTemplate> instance_template;
    if (impl->m_instance_template.IsEmpty()) {
        instance_template = ObjectTemplate::New(isolate);
        impl->m_instance_template = Copyable(ObjectTemplate)(isolate, instance_template);
        ObjectTemplateImpl *instt = V82JSC::ToImpl<ObjectTemplateImpl>(instance_template);
        instt->m_constructor_template = Copyable(FunctionTemplate)(isolate, thiz);
        instt->m_parent = Copyable(FunctionTemplate)(isolate, thiz);
    } else {
        instance_template = impl->m_instance_template.Get(isolate);
    }
    return instance_template;
}

/**
 * Causes the function template to inherit from a parent function template.
 * This means the the function's prototype.__proto__ is set to the parent
 * function's prototype.
 **/
void FunctionTemplate::Inherit(Local<FunctionTemplate> parent)
{
    Isolate* isolate = V82JSC::ToIsolate<FunctionTemplate>(this);
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    impl->m_parent = Copyable(FunctionTemplate)(isolate, parent);
}

/**
 * A PrototypeTemplate is the template used to create the prototype object
 * of the function created by this template.
 */
Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(impl));
    Local<ObjectTemplate> prototype_template;
    if (impl->m_prototype_template.IsEmpty()) {
        prototype_template = ObjectTemplate::New(isolate);
        impl->m_prototype_template = Copyable(ObjectTemplate)(isolate, prototype_template);
    } else {
        prototype_template = impl->m_prototype_template.Get(isolate);
    }
    return prototype_template;
}

/**
 * A PrototypeProviderTemplate is another function template whose prototype
 * property is used for this template. This is mutually exclusive with setting
 * a prototype template indirectly by calling PrototypeTemplate() or using
 * Inherit().
 **/
void FunctionTemplate::SetPrototypeProviderTemplate(Local<FunctionTemplate> prototype_provider)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    Isolate* isolate = V82JSC::ToIsolate(V82JSC::ToIsolateImpl(impl));
    impl->m_prototype_provider.Reset(isolate, prototype_provider);
}

/**
 * Set the class name of the FunctionTemplate.  This is used for
 * printing objects created with the function created from the
 * FunctionTemplate as its constructor.
 */
void FunctionTemplate::SetClassName(Local<String> name)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    impl->m_name.Reset(V82JSC::ToIsolate(this), name);
}

/**
 * When set to true, no access check will be performed on the receiver of a
 * function call.  Currently defaults to true, but this is subject to change.
 */
void FunctionTemplate::SetAcceptAnyReceiver(bool value)
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
void FunctionTemplate::SetHiddenPrototype(bool value)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    impl->m_isHiddenPrototype = value;
}

/**
 * Sets the ReadOnly flag in the attributes of the 'prototype' property
 * of functions created from this FunctionTemplate to true.
 */
void FunctionTemplate::ReadOnlyPrototype()
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    impl->m_readOnlyPrototype = true;
}

/**
 * Removes the prototype property from functions created from this
 * FunctionTemplate.
 */
void FunctionTemplate::RemovePrototype()
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    impl->m_removePrototype = true;
}

/**
 * Returns true if the given object is an instance of this function
 * template.
 */
bool FunctionTemplate::HasInstance(Local<Value> object)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    Isolate* isolate = V82JSC::ToIsolate(impl->GetIsolate());
    Local<Context> context = V82JSC::OperatingContext(isolate);
    if (!object->IsObject()) return false;
    
    JSValueRef o = V82JSC::ToJSValueRef(object, context);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    TrackedObjectImpl *wrap = getPrivateInstance(ctx, (JSObjectRef)o);
    if (!wrap) return false;
    
    if (!wrap->m_object_template.IsEmpty()) {
        ObjectTemplateImpl *ot = V82JSC::ToImpl<ObjectTemplateImpl>(wrap->m_object_template.Get(isolate));
        if (!ot->m_constructor_template.IsEmpty()) {
            return (V82JSC::ToImpl<FunctionTemplateImpl>(ot->m_constructor_template.Get(isolate)) == impl);
        }
    }
    return false;
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

JSValueRef FunctionTemplateImpl::callAsConstructorCallback(JSContextRef ctx,
                                                           JSObjectRef constructor_function,
                                                           JSObjectRef instance,
                                                           size_t argumentCount,
                                                           const JSValueRef *arguments,
                                                           JSValueRef *exception)
{
    IsolateImpl *isolateimpl = IsolateImpl::s_context_to_isolate_map[JSContextGetGlobalContext(ctx)];
    Isolate *isolate = V82JSC::ToIsolate(isolateimpl);
    Local<Context> context = LocalContextImpl::New(isolate, ctx);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    
    assert(argumentCount>0);
    bool create_object = JSValueToBoolean(ctx, arguments[0]);
    argumentCount--;
    arguments++;

    HandleScope scope(isolate);
    Context::Scope context_scope(context);
    
    void * private_data = JSObjectGetPrivate(constructor_function);
    Local<Template> templt = V82JSC::FromPersistentData<Template>(isolate, private_data);
    Local<v8::FunctionTemplate> function_template = * reinterpret_cast<Local<v8::FunctionTemplate>*>(&templt);
    FunctionTemplateImpl *ftempl = V82JSC::ToImpl<FunctionTemplateImpl>(function_template);
    Local<ObjectTemplate> instance_template = function_template->InstanceTemplate();
    ObjectTemplateImpl* otempl = V82JSC::ToImpl<ObjectTemplateImpl>(*instance_template);

    if (create_object) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        String::Utf8Value str(ftempl->m_name.Get(isolate));
        if (*str) {
            const std::string& name = *str;
            def.className = name.length() ? name.c_str() : nullptr;
        }
        if (otempl->m_callback) {
            def.callAsFunction = TemplateImpl::callAsFunctionCallback;
            def.callAsConstructor = TemplateImpl::callAsConstructorCallback;
        }

        void * data = V82JSC::PersistentData(isolate, instance_template);
        def.finalize = [](JSObjectRef object) {
            void * data = JSObjectGetPrivate(object);
            V82JSC::ReleasePersistentData<Template>(data);
        };

        JSClassRef claz = JSClassCreate(&def);
        instance = JSObjectMake(ctx, claz, data);

        JSObjectRef function = (JSObjectRef) V82JSC::ToJSValueRef(function_template->GetFunction(context).ToLocalChecked(), context);
        JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
        JSValueRef excp = 0;
        JSObjectRef prototype = (JSObjectRef) JSObjectGetProperty(ctx, function, sprototype, &excp);
        assert(excp == 0);
        V82JSC::SetRealPrototype(context, instance, prototype);
        assert(excp == 0);
    }
    
    TryCatch try_catch(isolate);
    
    Local<Object> thiz = otempl->NewInstance(context, instance, ftempl->m_isHiddenPrototype).ToLocalChecked();
    
    if (try_catch.HasCaught()) {
        if (exception) {
            *exception = V82JSC::ToJSValueRef<Value>(try_catch.Exception(), context);
        }
        return 0;
    }

    isolateimpl->m_callback_depth ++;

    Local<Value> data = ValueImpl::New(V82JSC::ToContextImpl(context), ftempl->m_data);
    typedef v8::internal::Heap::RootListIndex R;
    internal::Object *the_hole = isolateimpl->ii.heap()->root(R::kTheHoleValueRootIndex);

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 0;
        O(isolateimpl),                                      // kIsolateIndex = 1;
        the_hole,                                            // kReturnValueDefaultValueIndex = 2;
        the_hole,                                            // kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        * reinterpret_cast<v8::internal::Object**>(*thiz)    // kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<Value> arg = ValueImpl::New(ctximpl, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    FunctionCallbackImpl info(implicit, values, (int) argumentCount);
    
    isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;

    if (ftempl->m_callback) {
        ftempl->m_callback(info);
    }
    
    if (try_catch.HasCaught()) {
        *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
    } else if (isolateimpl->ii.thread_local_top()->scheduled_exception_ != the_hole) {
        internal::Object * excep = isolateimpl->ii.thread_local_top()->scheduled_exception_;
        *exception = V82JSC::ToJSValueRef_<Value>(excep, context);
        isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    }

    -- isolateimpl->m_callback_depth;
    
    if (implicit[3] == the_hole) {
        return V82JSC::ToJSValueRef<Object>(thiz, context);
    }

    Local<Value> ret = info.GetReturnValue().Get();

    return (JSObjectRef) V82JSC::ToJSValueRef<Value>(ret, isolate);
}

