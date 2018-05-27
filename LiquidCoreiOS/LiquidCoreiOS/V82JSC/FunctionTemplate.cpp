//
//  FunctionTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

#define PROXY_CALL_MAGIC_NUMBER ((void*)0x0133707eel)

static void signatureDestructor(InternalObjectImpl* o)
{
    static_cast<SignatureImpl*>(o)->m_template.Reset();
}

Local<Signature> Signature::New(Isolate* isolate,
                                Local<FunctionTemplate> receiver)
{
    SignatureImpl * signature = static_cast<SignatureImpl *>(HeapAllocator::Alloc(reinterpret_cast<IsolateImpl*>(isolate),
                                                                                  sizeof(SignatureImpl),
                                                                                  signatureDestructor));
    signature->m_template = Copyable(FunctionTemplate)(isolate, receiver);
    
    return V82JSC::CreateLocal<Signature>(isolate, signature);
}

Local<AccessorSignature> AccessorSignature::New(Isolate* isolate,
                                                Local<FunctionTemplate> receiver)
{
    SignatureImpl * signature = static_cast<SignatureImpl *>(HeapAllocator::Alloc(reinterpret_cast<IsolateImpl*>(isolate),
                                                                                  sizeof(SignatureImpl),
                                                                                  signatureDestructor));
    signature->m_template = Copyable(FunctionTemplate)(isolate, receiver);
    
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
    auto destructor = [](InternalObjectImpl *o)
    {
        FunctionTemplateImpl *templ = static_cast<FunctionTemplateImpl*>(o);
        for (auto i=templ->m_functions.begin(); i!=templ->m_functions.end(); ++i) {
            JSValueUnprotect(i->first, i->second);
        }
        templ->m_functions.clear();
        templ->m_name.clear();
        templateDestructor(o);
    };
    Local<Context> context = V82JSC::OperatingContext(isolate);
    FunctionTemplateImpl *templ = (FunctionTemplateImpl*) TemplateImpl::New(isolate, sizeof(FunctionTemplateImpl), destructor);
    V82JSC::Map(templ)->set_instance_type(v8::internal::FUNCTION_TEMPLATE_INFO_TYPE);

    templ->m_name = std::string();
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
    templ->m_functions = std::map<JSContextRef, JSObjectRef>();
    
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

    Local<FunctionTemplate> thiz = V82JSC::CreateLocal<FunctionTemplate>(isolate, impl);

    JSObjectRef function = impl->m_functions[JSContextGetGlobalContext(ctx)];
    if (function) {
        return ValueImpl::New(ctximpl, function).As<Function>();
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

    TemplateWrap *wrap = new TemplateWrap();
    wrap->m_template.Reset(isolate, thiz);
    wrap->m_isolate = iso;
    
    JSClassDefinition function_def = kJSClassDefinitionEmpty;
    function_def.callAsFunction = TemplateImpl::callAsFunctionCallback;
    function_def.className = "function_proxy";
    function_def.finalize = [](JSObjectRef object) {
        TemplateWrap *wrap = (TemplateWrap*) JSObjectGetPrivate(object);
        if (--wrap->m_count == 0) {
            delete wrap;
        }
    };
    JSClassRef function_class = JSClassCreate(&function_def);
    JSObjectRef function_proxy = JSObjectMake(ctx, function_class, (void*)wrap);
    V82JSC::SetRealPrototype(context, function_proxy, generic_function_prototype);
    JSClassRelease(function_class);

    JSClassDefinition constructor_def = kJSClassDefinitionEmpty;
    constructor_def.callAsFunction = FunctionTemplateImpl::callAsConstructorCallback;
    constructor_def.className = "constructor_proxy";
    constructor_def.finalize = [](JSObjectRef object) {
        TemplateWrap *wrap = (TemplateWrap*) JSObjectGetPrivate(object);
        if (--wrap->m_count == 0) {
            delete wrap;
        }
    };
    JSClassRef constructor_class = JSClassCreate(&constructor_def);
    JSObjectRef constructor_proxy = JSObjectMake(ctx, constructor_class, (void*)wrap);
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
    const char *sname = impl->m_name.length() ? impl->m_name.c_str() : "Function";
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
    
    V82JSC::makePrivateInstance(iso, ctx, function);
    
    LocalException exception(iso);

    MaybeLocal<Object> thizo = impl->InitInstance(context, function, exception);
    if (thizo.IsEmpty()) {
        return MaybeLocal<Function>();
    }
    if (!impl->m_removePrototype) {
        JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
        Local<ObjectTemplate> prototype_template = thiz->PrototypeTemplate();
        MaybeLocal<Object> prototype = prototype_template->NewInstance(context);
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

    impl->m_functions[ctx] = function;
    JSValueProtect(ctx, function);
    return ValueImpl::New(ctximpl, function).As<Function>();
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
    assert(0);
}

/**
 * Set the class name of the FunctionTemplate.  This is used for
 * printing objects created with the function created from the
 * FunctionTemplate as its constructor.
 */
void FunctionTemplate::SetClassName(Local<String> name)
{
    FunctionTemplateImpl *impl = V82JSC::ToImpl<FunctionTemplateImpl,FunctionTemplate>(this);
    String::Utf8Value str(name);
    impl->m_name = std::string(*str);
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
    assert(0);
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
    TemplateWrap *wrap = reinterpret_cast<TemplateWrap*>(JSObjectGetPrivate(constructor_function));
    IsolateImpl *isolateimpl = wrap->m_isolate;
    Isolate *isolate = V82JSC::ToIsolate(isolateimpl);
    Local<Context> context = ContextImpl::New(isolate, ctx);
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    
    assert(argumentCount>0);
    bool create_object = JSValueToBoolean(ctx, arguments[0]);
    argumentCount--;
    arguments++;

    HandleScope scope(isolate);
    
    Local<Template> templt = wrap->m_template.Get(isolate);
    Local<FunctionTemplate> function_template = * reinterpret_cast<Local<FunctionTemplate>*>(&templt);
    FunctionTemplateImpl *ftempl = V82JSC::ToImpl<FunctionTemplateImpl>(function_template);
    Local<ObjectTemplate> instance_template = function_template->InstanceTemplate();
    ObjectTemplateImpl* otempl = V82JSC::ToImpl<ObjectTemplateImpl>(*instance_template);

    if (create_object) {
        JSClassDefinition def = kJSClassDefinitionEmpty;
        def.attributes = kJSClassAttributeNoAutomaticPrototype;
        const std::string& name = ftempl->m_name;
        def.className = name.length() ? name.c_str() : nullptr;
        if (otempl->m_callback) {
            def.callAsFunction = TemplateImpl::callAsFunctionCallback;
            def.callAsConstructor = TemplateImpl::callAsConstructorCallback;
        }
        TemplateWrap *wrap = new TemplateWrap();
        wrap->m_template.Reset(isolate, instance_template);
        wrap->m_isolate = isolateimpl;
        def.finalize = [](JSObjectRef obj) {
            TemplateWrap* wrap = (TemplateWrap*) JSObjectGetPrivate(obj);
            wrap->m_template.Reset();
            delete wrap;
        };

        JSClassRef claz = JSClassCreate(&def);
        instance = JSObjectMake(ctx, claz, wrap);

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
        if (excep->IsHeapObject()) {
            ValueImpl* i = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(excep) - internal::kHeapObjectTag);
            *exception = i->m_value;
        } else {
            *exception = JSValueMakeNumber(ctx, internal::Smi::ToInt(excep));
        }
        isolateimpl->ii.thread_local_top()->scheduled_exception_ = the_hole;
    }

    if (implicit[3] == the_hole) {
        return V82JSC::ToJSValueRef<Object>(thiz, context);
    }

    Local<Value> ret = info.GetReturnValue().Get();
    return (JSObjectRef) V82JSC::ToJSValueRef<Value>(ret, isolate);
}

