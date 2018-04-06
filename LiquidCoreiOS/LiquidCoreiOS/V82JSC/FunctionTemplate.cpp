//
//  FunctionTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

SignatureImpl::SignatureImpl()
{
    
}

Local<Signature> Signature::New(Isolate* isolate,
                                Local<FunctionTemplate> receiver)
{
    SignatureImpl *signature = new SignatureImpl();
    signature->m_isolate = isolate;
    signature->m_template = reinterpret_cast<ObjectTemplateImpl*>(*receiver);
    
    return _local<Signature>(signature).toLocal();
}

Local<AccessorSignature> AccessorSignature::New(Isolate* isolate,
                                                Local<FunctionTemplate> receiver)
{
    return Local<AccessorSignature>();
}

#undef O
#define O(v) reinterpret_cast<v8::internal::Object*>(v)

JSObjectRef ObjectTemplateImpl::callAsConstructorCallback(JSContextRef ctx,
                                                          JSObjectRef constructor,
                                                          size_t argumentCount,
                                                          const JSValueRef *arguments,
                                                          JSValueRef *exception)
{
    ObjectTemplateWrap *wrap = reinterpret_cast<ObjectTemplateWrap*>(JSObjectGetPrivate(constructor));
    Isolate *isolate = reinterpret_cast<Isolate*>(wrap->m_context->isolate);
    Local<ObjectTemplate> templ = _local<ObjectTemplate>(const_cast<ObjectTemplateImpl*>(wrap->m_template)).toLocal();
    Local<Context> context = _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal();

    TryCatch try_catch(isolate);
    MaybeLocal<Object> maybe_thiz = templ->NewInstance(context);
    if (maybe_thiz.IsEmpty()) {
        if (exception) {
            *exception = V82JSC::ToJSValueRef(try_catch.Exception(), context);
        }
        return 0;
    }
    Local<Object> thiz = maybe_thiz.ToLocalChecked();
    JSObjectRef thisObject = (JSObjectRef) V82JSC::ToJSValueRef(thiz, context);
    
    JSStringRef ctor = JSStringCreateWithUTF8CString("constructor");
    JSValueRef excp = nullptr;
    JSObjectSetProperty(ctx, thisObject, ctor, constructor, kJSPropertyAttributeDontEnum, &excp);
    JSStringRelease(ctor);
    assert(excp==nullptr);

    // FIXME: Whatever needs to be done to make instanceof work, signature, etc.
    
    Local<Value> data = ValueImpl::New(wrap->m_context, wrap->m_template->m_data);

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 0;
        O(wrap->m_context->isolate),                         // kIsolateIndex = 1;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueDefaultValueIndex = 2;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        * reinterpret_cast<v8::internal::Object**>(*thiz)    // kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<Value> arg = ValueImpl::New(wrap->m_context, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }
    
    FunctionCallbackImpl info(implicit, values, (int) argumentCount);
    
    if (wrap->m_template->m_callback) {
        wrap->m_template->m_callback(info);
    }
    
    if(!implicit[3]->IsUndefined(reinterpret_cast<v8::internal::Isolate*>(isolate))) {
        Local<Value> ret = info.GetReturnValue().Get();
        return (JSObjectRef) V82JSC::ToJSValueRef<Value>(ret, isolate);
    }
    
    return thisObject;
}

JSValueRef ObjectTemplateImpl::callAsFunctionCallback(JSContextRef ctx,
                                                      JSObjectRef function,
                                                      JSObjectRef thisObject,
                                                      size_t argumentCount,
                                                      const JSValueRef *arguments,
                                                      JSValueRef *exception)
{
    ObjectTemplateWrap *wrap = reinterpret_cast<ObjectTemplateWrap*>(JSObjectGetPrivate(function));

    Local<Value> thiz = ValueImpl::New(wrap->m_context, thisObject);
    
    // Check signature
    bool signature_match = !wrap->m_template->m_signature;
    if (!signature_match) {
        ObjectTemplateWrap *thisWrap =
            reinterpret_cast<ObjectTemplateWrap*>(JSObjectGetPrivate(thisObject));
        SignatureImpl *sig = wrap->m_template->m_signature;
        
        while (thisWrap) {
            if (sig->m_template == thisWrap->m_template) {
                signature_match = true;
                break;
            }
            JSValueRef proto = JSObjectGetPrototype(ctx, thisObject);
            if (JSValueIsObject(ctx, proto)) {
                thisWrap = reinterpret_cast<ObjectTemplateWrap*>(JSObjectGetPrivate((JSObjectRef)proto));
            } else {
                thisWrap = nullptr;
            }
        }
    }
    if (!signature_match) {
        JSStringRef message = JSStringCreateWithUTF8CString("new TypeError('Illegal invocation')");
        *exception = JSEvaluateScript(ctx, message, 0, 0, 0, 0);
        JSStringRelease(message);
        return 0;
    }
    Local<Value> data = ValueImpl::New(wrap->m_context, wrap->m_template->m_data);

    v8::internal::Object * implicit[] = {
        * reinterpret_cast<v8::internal::Object**>(*thiz),   // kHolderIndex = 0;
        O(wrap->m_context->isolate),                         // kIsolateIndex = 1;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueDefaultValueIndex = 2;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kReturnValueIndex = 3;
        * reinterpret_cast<v8::internal::Object**>(*data),   // kDataIndex = 4;
        nullptr /*deprecated*/,                              // kCalleeIndex = 5;
        nullptr, // FIXME                                    // kContextSaveIndex = 6;
        O(wrap->m_context->isolate->i.roots.undefined_value),// kNewTargetIndex = 7;
    };
    v8::internal::Object * values_[argumentCount + 1];
    v8::internal::Object ** values = values_ + argumentCount - 1;
    *(values + 1) = * reinterpret_cast<v8::internal::Object**>(*thiz);
    for (size_t i=0; i<argumentCount; i++) {
        Local<Value> arg = ValueImpl::New(wrap->m_context, arguments[i]);
        *(values-i) = * reinterpret_cast<v8::internal::Object**>(*arg);
    }

    FunctionCallbackImpl info(implicit, values, (int) argumentCount);

    if (wrap->m_template->m_callback) {
        wrap->m_template->m_callback(info);
    }

    Local<Value> ret = info.GetReturnValue().Get();
    
    return V82JSC::ToJSValueRef<Value>(ret, _local<Context>(const_cast<ContextImpl*>(wrap->m_context)).toLocal());
}

/** Creates a function template.*/
Local<FunctionTemplate> FunctionTemplate::New(Isolate* isolate, FunctionCallback callback,
                                              Local<Value> data,
                                              Local<Signature> signature, int length,
                                              ConstructorBehavior behavior)
{
    ObjectTemplateImpl *templ = (ObjectTemplateImpl *) malloc(sizeof (ObjectTemplateImpl));
    memset(templ, 0, sizeof(ObjectTemplateImpl));
    templ->pMap = (v8::internal::Map *)((reinterpret_cast<intptr_t>(&templ->map) & ~3) + 1);
    templ->pMap->set_instance_type(v8::internal::FUNCTION_TEMPLATE_INFO_TYPE);

    templ->m_properties = std::map<JSStringRef, ValueImpl*>();
    templ->m_property_accessors = std::map<JSStringRef, PropAccessor>();
    templ->m_obj_accessors = std::map<JSStringRef, ObjAccessor>();
    templ->m_name = std::string();
    templ->m_isolate = isolate;
    templ->m_callback = callback;
    templ->m_signature = reinterpret_cast<SignatureImpl*>(*signature);
    templ->m_behavior = behavior;
    templ->m_length = length;
    if (!*data) {
        data = Undefined(isolate);
    }
    templ->m_data = V82JSC::ToJSValueRef<Value>(data, isolate);
    JSValueProtect(V82JSC::ToIsolateImpl(isolate)->m_defaultContext->m_context, templ->m_data);
    
    templ->m_definition = kJSClassDefinitionEmpty;
    templ->m_definition.callAsFunction = ObjectTemplateImpl::callAsFunctionCallback;
    if (behavior == ConstructorBehavior::kAllow) {
        templ->m_definition.callAsConstructor = ObjectTemplateImpl::callAsConstructorCallback;
    }
    if (templ->m_signature) {
        templ->m_definition.parentClass = templ->m_signature->m_template->m_class;
    }
    templ->m_definition.attributes = kJSClassAttributeNoAutomaticPrototype;
    
    return _local<FunctionTemplate>(templ).toLocal();
}

/** Get a template included in the snapshot by index. */
MaybeLocal<FunctionTemplate> FunctionTemplate::FromSnapshot(Isolate* isolate,
                                                            size_t index)
{
    return MaybeLocal<FunctionTemplate>();
}

/**
 * Creates a function template backed/cached by a private property.
 */
Local<FunctionTemplate> FunctionTemplate::NewWithCache(
                                            Isolate* isolate, FunctionCallback callback,
                                            Local<Private> cache_property, Local<Value> data,
                                            Local<Signature> signature, int length)
{
    return Local<FunctionTemplate>();
}

/** Returns the unique function instance in the current execution context.*/
MaybeLocal<Function> FunctionTemplate::GetFunction(Local<Context> context)
{
    ObjectTemplateImpl *impl =  V82JSC::ToImpl<ObjectTemplateImpl,FunctionTemplate>(this);
    JSObjectRef function = impl->m_function;
    if (function) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), function).As<Function>();
    }
    
    const ContextImpl *ctx = V82JSC::ToContextImpl(context);
    
    impl->m_class = JSClassCreate(&impl->m_definition);
    
    ObjectTemplateWrap *wrap = new ObjectTemplateWrap();
    wrap->m_template = impl;
    wrap->m_context = ctx;
    
    LocalException exception(ctx->isolate);
    JSObjectRef instance = JSObjectMake(ctx->m_context, impl->m_class, (void*)wrap);
    MaybeLocal<Object> v = impl->InitInstance(context, instance, exception);
    if (!v.IsEmpty()) {
        impl->m_function = (JSObjectRef) V82JSC::ToJSValueRef<Object>(v.ToLocalChecked(), context);
        JSValueProtect(V82JSC::ToContextRef(context), impl->m_function);
    }
    return * reinterpret_cast<MaybeLocal<Function> *>(reinterpret_cast<void *>(&v));
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
    ObjectTemplateImpl *impl = reinterpret_cast<ObjectTemplateImpl*>(this);
    impl->m_callback = callback;
    if (!*data) {
        data = Undefined(impl->m_isolate);
    }
    impl->m_data = V82JSC::ToJSValueRef(data, impl->m_isolate);
}

/** Set the predefined length property for the FunctionTemplate. */
void FunctionTemplate::SetLength(int length)
{
    ObjectTemplateImpl *impl = reinterpret_cast<ObjectTemplateImpl*>(this);
    impl->m_length = length;
}

/** Get the InstanceTemplate. */
Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
    return _local<ObjectTemplate>(this).toLocal();
}

/**
 * Causes the function template to inherit from a parent function template.
 * This means the the function's prototype.__proto__ is set to the parent
 * function's prototype.
 **/
void FunctionTemplate::Inherit(Local<FunctionTemplate> parent)
{
    ObjectTemplateImpl *impl = reinterpret_cast<ObjectTemplateImpl*>(this);
    ObjectTemplateImpl *p = reinterpret_cast<ObjectTemplateImpl*>(*parent);
    impl->m_parent = p;
}

/**
 * A PrototypeTemplate is the template used to create the prototype object
 * of the function created by this template.
 */
Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
    ObjectTemplateImpl *this_ = static_cast<ObjectTemplateImpl*>(reinterpret_cast<ValueImpl*>(this));
    if (!this_->m_parent) {
        Local<FunctionTemplate> p = this->New(this_->m_isolate);
        this_->m_parent = static_cast<ObjectTemplateImpl*>(reinterpret_cast<ValueImpl*>(*p));
    }
    return _local<ObjectTemplate>(this_->m_parent).toLocal();
}

/**
 * A PrototypeProviderTemplate is another function template whose prototype
 * property is used for this template. This is mutually exclusive with setting
 * a prototype template indirectly by calling PrototypeTemplate() or using
 * Inherit().
 **/
void FunctionTemplate::SetPrototypeProviderTemplate(Local<FunctionTemplate> prototype_provider)
{
    
}

/**
 * Set the class name of the FunctionTemplate.  This is used for
 * printing objects created with the function created from the
 * FunctionTemplate as its constructor.
 */
void FunctionTemplate::SetClassName(Local<String> name)
{
    ObjectTemplateImpl *this_ = V82JSC::ToImpl<ObjectTemplateImpl,FunctionTemplate>(this);
    String::Utf8Value str(name);
    this_->m_name = std::string(*str);
    this_->m_definition.className = this_->m_name.c_str();
}

/**
 * When set to true, no access check will be performed on the receiver of a
 * function call.  Currently defaults to true, but this is subject to change.
 */
void FunctionTemplate::SetAcceptAnyReceiver(bool value)
{
    
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
    
}

/**
 * Sets the ReadOnly flag in the attributes of the 'prototype' property
 * of functions created from this FunctionTemplate to true.
 */
void FunctionTemplate::ReadOnlyPrototype()
{
    
}

/**
 * Removes the prototype property from functions created from this
 * FunctionTemplate.
 */
void FunctionTemplate::RemovePrototype()
{
    
}

/**
 * Returns true if the given object is an instance of this function
 * template.
 */
bool FunctionTemplate::HasInstance(Local<Value> object)
{
    return false;
}
