//
//  Context.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Context.h"
#include "Isolate.h"
#include "Utils.h"

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
    return Utils::NewObject(nullptr);
}

/**
 * Detaches the global object from its context before
 * the global object can be reused to create a new context.
 */
void Context::DetachGlobal()
{
    
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
    
    context->m_context = JSGlobalContextCreateInGroup(((IsolateImpl *)isolate)->m_group, nullptr);
    
    return Local<Context>(context);
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
    return MaybeLocal<Context>();
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
    return MaybeLocal<Object>();
}

/**
 * Sets the security token for the context.  To access an object in
 * another context, the security tokens must match.
 */
void Context::SetSecurityToken(Local<Value> token)
{
    
}

/** Restores the security token to the default value. */
void Context::UseDefaultSecurityToken()
{
    
}

/** Returns the security token of this context.*/
Local<Value> Context::GetSecurityToken()
{
    return Utils::NewValue(nullptr);
}

/**
 * Enter this context.  After entering a context, all code compiled
 * and run is compiled and run in this context.  If another context
 * is already entered, this old context is saved so it can be
 * restored when the new context is exited.
 */
void Context::Enter()
{
    
}

/**
 * Exit this context.  Exiting the current context restores the
 * context that was in place when entering the current context.
 */
void Context::Exit()
{
    
}

/** Returns an isolate associated with a current context. */
Isolate* Context::GetIsolate()
{
    return nullptr;
}

/**
 * Gets the binding object used by V8 extras. Extra natives get a reference
 * to this object and can use it to "export" functionality by adding
 * properties. Extra natives can also "import" functionality by accessing
 * properties added by the embedder using the V8 API.
 */
Local<Object> Context::GetExtrasBindingObject()
{
    return Utils::NewObject(nullptr);
}

/**
 * Sets the embedder data with the given index, growing the data as
 * needed. Note that index 0 currently has a special meaning for Chrome's
 * debugger.
 */
void Context::SetEmbedderData(int index, Local<Value> value)
{
    
}

/**
 * Sets a 2-byte-aligned native pointer in the embedder data with the given
 * index, growing the data as needed. Note that index 0 currently has a
 * special meaning for Chrome's debugger.
 */
void Context::SetAlignedPointerInEmbedderData(int index, void* value)
{
    
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
    
}
