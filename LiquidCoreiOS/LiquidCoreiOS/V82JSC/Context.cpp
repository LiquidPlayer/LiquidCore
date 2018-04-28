//
//  Context.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/**
 * Returns the global proxy object.
 *
 * Global proxy object is a thin wrapper whose prototype points to actual
 * context's global object with the properties like Object, etc. This is done
 * that way for security reasons (for more details see
 * https://wiki.mozilla.org/Gecko:SplitWindow).
 *
 * Please note that changes to global proxy object prototype most probably
 * would break VM---v8 expects only global object as a prototype of global
 * proxy object.
 */
Local<Object> Context::Global()
{
    ContextImpl *impl = reinterpret_cast<ContextImpl *>(this);
    JSObjectRef glob = JSContextGetGlobalObject(impl->m_ctxRef);
    return ValueImpl::New(impl, glob).As<Object>();
}

/**
 * Detaches the global object from its context before
 * the global object can be reused to create a new context.
 */
void Context::DetachGlobal()
{
    //FIXME: Not sure what to do here to ensure it isn't used in the existing context
    //assert(0);
}

Local<Context> ContextImpl::New(Isolate *isolate, JSContextRef ctx)
{
    ContextImpl * context = (ContextImpl *) malloc(sizeof (ContextImpl));
    memset(context, 0, sizeof(ContextImpl));
    context->pMap = reinterpret_cast<internal::Map *>(reinterpret_cast<uint8_t*>(context) + internal::kHeapObjectTag);
    IsolateImpl * i = reinterpret_cast<IsolateImpl*>(isolate);
    context->m_isolate = i;
    context->m_ctxRef = ctx;
    
    return _local<Context>(context).toLocal();
}


/**
 * Creates a new context and returns a handle to the newly allocated
 * context.
 *
 * \param isolate The isolate in which to create the context.
 *
 * \param extensions An optional extension configuration containing
 * the extensions to be installed in the newly created context.
 *
 * \param global_template An optional object template from which the
 * global object for the newly created context will be created.
 *
 * \param global_object An optional global object to be reused for
 * the newly created context. This global object must have been
 * created by a previous call to Context::New with the same global
 * template. The state of the global object will be completely reset
 * and only object identify will remain.
 */
Local<Context> Context::New(Isolate* isolate, ExtensionConfiguration* extensions,
                          MaybeLocal<ObjectTemplate> global_template,
                          MaybeLocal<Value> global_object)
{
    ContextImpl * context = (ContextImpl *) malloc(sizeof (ContextImpl));
    memset(context, 0, sizeof(ContextImpl));
    context->pMap = reinterpret_cast<internal::Map*>(reinterpret_cast<uint8_t*>(context) + 1);
    IsolateImpl * i = reinterpret_cast<IsolateImpl*>(isolate);
    context->m_isolate = i;
    Local<Context> ctx = _local<Context>(context).toLocal();
    int hash = 0;
    context->m_loaded_extensions = std::map<std::string, bool>();
    
    if (!global_object.IsEmpty()) {
        hash = global_object.ToLocalChecked().As<Object>()->GetIdentityHash();
    }

    if (!global_template.IsEmpty()) {
        ObjectTemplateImpl *impl = V82JSC::ToImpl<ObjectTemplateImpl>(*global_template.ToLocalChecked());
        
        LocalException exception(V82JSC::ToIsolateImpl(isolate));
        
        context->m_ctxRef = JSGlobalContextCreateInGroup(i->m_group, nullptr);
        JSObjectRef global = JSContextGetGlobalObject(context->m_ctxRef);
        JSObjectRef instance = JSObjectMake(context->m_ctxRef, 0, nullptr);
        JSObjectSetPrototype(context->m_ctxRef, global, instance);

        if (impl->m_constructor_template) {
            MaybeLocal<Function> ctor = _local<FunctionTemplate>(impl->m_constructor_template).toLocal()->GetFunction(ctx);
            if (!ctor.IsEmpty()) {
                JSObjectRef ctor_func = (JSObjectRef) V82JSC::ToJSValueRef(ctor.ToLocalChecked(), ctx);
                JSStringRef sprototype = JSStringCreateWithUTF8CString("prototype");
                JSStringRef sconstructor = JSStringCreateWithUTF8CString("constructor");
                JSValueRef excp = 0;
                JSValueRef prototype = JSObjectGetProperty(context->m_ctxRef, ctor_func, sprototype, &excp);
                assert(excp == 0);
                JSObjectSetPrototype(context->m_ctxRef, instance, prototype);
                JSObjectSetProperty(context->m_ctxRef, instance, sconstructor, ctor_func, kJSPropertyAttributeDontEnum, &excp);
                assert(excp == 0);
                JSStringRelease(sprototype);
                JSStringRelease(sconstructor);
            }
        }
        impl->NewInstance(ctx, instance);
        if (hash) {
            V82JSC::getPrivateInstance(context->m_ctxRef, instance)->m_hash = hash;
        }

        if (exception.ShouldThow()) {
            return Local<Context>();
        }
    } else {
        context->m_ctxRef = JSGlobalContextCreateInGroup(i->m_group, nullptr);
    }
    
    // Don't do anything fancy if we are setting up the default context
    if (i->m_nullContext != nullptr) {
        proxyArrayBuffer(context);

        InstallAutoExtensions(ctx);
        if (extensions) {
            for (const char **extension = extensions->begin(); extension != extensions->end(); extension++) {
                if (!InstallExtension(ctx, *extension)) {
                    return Local<Context>();
                }
            }
        }
    }
    
    return ctx;
}

/**
 * Create a new context from a (non-default) context snapshot. There
 * is no way to provide a global object template since we do not create
 * a new global object from template, but we can reuse a global object.
 *
 * \param isolate See v8::Context::New.
 *
 * \param context_snapshot_index The index of the context snapshot to
 * deserialize from. Use v8::Context::New for the default snapshot.
 *
 * \param embedder_fields_deserializer Optional callback to deserialize
 * internal fields. It should match the SerializeInternalFieldCallback used
 * to serialize.
 *
 * \param extensions See v8::Context::New.
 *
 * \param global_object See v8::Context::New.
 */

MaybeLocal<Context> Context::FromSnapshot(
                                        Isolate* isolate, size_t context_snapshot_index,
                                        DeserializeInternalFieldsCallback embedder_fields_deserializer,
                                        ExtensionConfiguration* extensions,
                                        MaybeLocal<Value> global_object)
{
    // Snashots are ignored
    Local<Context> ctx = New(isolate, extensions, MaybeLocal<ObjectTemplate>(), global_object);
    return MaybeLocal<Context>(ctx);
}

/**
 * Returns an global object that isn't backed by an actual context.
 *
 * The global template needs to have access checks with handlers installed.
 * If an existing global object is passed in, the global object is detached
 * from its context.
 *
 * Note that this is different from a detached context where all accesses to
 * the global proxy will fail. Instead, the access check handlers are invoked.
 *
 * It is also not possible to detach an object returned by this method.
 * Instead, the access check handlers need to return nothing to achieve the
 * same effect.
 *
 * It is possible, however, to create a new context from the global object
 * returned by this method.
 */
MaybeLocal<Object> Context::NewRemoteContext(
                                           Isolate* isolate, Local<ObjectTemplate> global_template,
                                           MaybeLocal<Value> global_object)
{
    assert(0);
    return MaybeLocal<Object>();
}

/**
 * Sets the security token for the context.  To access an object in
 * another context, the security tokens must match.
 */
void Context::SetSecurityToken(Local<Value> token)
{
    assert(0);
}

/** Restores the security token to the default value. */
void Context::UseDefaultSecurityToken()
{
    assert(0);
}

/** Returns the security token of this context.*/
Local<Value> Context::GetSecurityToken()
{
    assert(0);
    return Local<Value>();
}

/**
 * Enter this context.  After entering a context, all code compiled
 * and run is compiled and run in this context.  If another context
 * is already entered, this old context is saved so it can be
 * restored when the new context is exited.
 */
void Context::Enter()
{
    ContextImpl *impl = V82JSC::ToContextImpl(this);

    impl->m_isolate->EnterContext(this);
}

/**
 * Exit this context.  Exiting the current context restores the
 * context that was in place when entering the current context.
 */
void Context::Exit()
{
    ContextImpl *impl = V82JSC::ToContextImpl(this);
    
    impl->m_isolate->ExitContext(this);
}

/** Returns an isolate associated with a current context. */
Isolate* Context::GetIsolate()
{
    ContextImpl *impl = V82JSC::ToContextImpl(this);
    
    return V82JSC::ToIsolate(impl->m_isolate);
}

/**
 * Gets the binding object used by V8 extras. Extra natives get a reference
 * to this object and can use it to "export" functionality by adding
 * properties. Extra natives can also "import" functionality by accessing
 * properties added by the embedder using the V8 API.
 */
Local<Object> Context::GetExtrasBindingObject()
{
    assert(0);
    return Local<Object>();
}

template<typename T>
static void WriteField(internal::Object* ptr, int offset, T value) {
    uint8_t* addr =
        reinterpret_cast<uint8_t*>(ptr) + offset - internal::kHeapObjectTag;
    *reinterpret_cast<T*>(addr) = value;
}

/**
 * Sets the embedder data with the given index, growing the data as
 * needed. Note that index 0 currently has a special meaning for Chrome's
 * debugger.
 */
void Context::SetEmbedderData(int index, Local<Value> value)
{
    typedef internal::Object O;
    O* val = *reinterpret_cast<O* const*>(*value);
    SetAlignedPointerInEmbedderData(index, val);
}

/**
 * Sets a 2-byte-aligned native pointer in the embedder data with the given
 * index, growing the data as needed. Note that index 0 currently has a
 * special meaning for Chrome's debugger.
 */
void Context::SetAlignedPointerInEmbedderData(int index, void* value)
{
    typedef internal::Object O;
    typedef internal::Internals I;
    O* ctx = *reinterpret_cast<O* const*>(this);
    int embedder_data_offset = I::kContextHeaderSize +
        (internal::kApiPointerSize * I::kContextEmbedderDataIndex);
    O* embedder_data = I::ReadField<O*>(ctx, embedder_data_offset);
    O** copy = nullptr;
    int copy_pointers = 0;
    void *defunct = nullptr;
    if (embedder_data) {
        EmbedderDataImpl *ed = reinterpret_cast<EmbedderDataImpl*>(reinterpret_cast<uint8_t*>(embedder_data) - internal::kHeapObjectTag);
        if (ed->m_size <= index) {
            copy = &ed->m_embedder_data[0];
            copy_pointers = ed->m_size;
            embedder_data = nullptr;
            defunct = ed;
        }
    }
    if (!embedder_data) {
        int size = ((index + 32) / 32) * 32;
        EmbedderDataImpl* io = (EmbedderDataImpl*) malloc(sizeof(EmbedderDataImpl) + size * internal::kApiPointerSize);
        memset(io, 0, sizeof(InternalObjectImpl) + size * internal::kApiPointerSize);
        io->pMap = (v8::internal::Map*)(reinterpret_cast<uintptr_t>(io) + 1);
        io->m_size = size;
        memcpy(&io->m_embedder_data[0], copy, copy_pointers * internal::kApiPointerSize);
        /* FIXME if (defunct) free(defunct); */
        embedder_data = reinterpret_cast<O*>(io->pMap);
        WriteField<O*>(ctx, embedder_data_offset, embedder_data);
    }
    int value_offset =
        I::kFixedArrayHeaderSize + (internal::kApiPointerSize * index);
    WriteField<void*>(embedder_data, value_offset, value);
}

/**
 * Control whether code generation from strings is allowed. Calling
 * this method with false will disable 'eval' and the 'Function'
 * constructor for code running in this context. If 'eval' or the
 * 'Function' constructor are used an exception will be thrown.
 *
 * If code generation from strings is not allowed the
 * V8::AllowCodeGenerationFromStrings callback will be invoked if
 * set before blocking the call to 'eval' or the 'Function'
 * constructor. If that callback returns true, the call will be
 * allowed, otherwise an exception will be thrown. If no callback is
 * set an exception will be thrown.
 */
void Context::AllowCodeGenerationFromStrings(bool allow)
{
    assert(0);
}

/**
 * Returns true if code generation from strings is allowed for the context.
 * For more details see AllowCodeGenerationFromStrings(bool) documentation.
 */
bool Context::IsCodeGenerationFromStringsAllowed()
{
    return false;
}

/**
 * Sets the error description for the exception that is thrown when
 * code generation from strings is not allowed and 'eval' or the 'Function'
 * constructor are called.
 */
void Context::SetErrorMessageForCodeGenerationFromStrings(Local<String> message)
{
    assert(0);
}
