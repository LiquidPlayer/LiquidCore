//
//  FunctionTemplate.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/** Creates a function template.*/
Local<FunctionTemplate> FunctionTemplate::New(Isolate* isolate, FunctionCallback callback,
                                              Local<Value> data,
                                              Local<Signature> signature, int length,
                                              ConstructorBehavior behavior)
{
    return Local<FunctionTemplate>();
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
    return MaybeLocal<Function>();
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
    
}

/** Set the predefined length property for the FunctionTemplate. */
void FunctionTemplate::SetLength(int length)
{
    
}

/** Get the InstanceTemplate. */
Local<ObjectTemplate> FunctionTemplate::InstanceTemplate()
{
    return Local<ObjectTemplate>();
}

/**
 * Causes the function template to inherit from a parent function template.
 * This means the the function's prototype.__proto__ is set to the parent
 * function's prototype.
 **/
void FunctionTemplate::Inherit(Local<FunctionTemplate> parent)
{
    
}

/**
 * A PrototypeTemplate is the template used to create the prototype object
 * of the function created by this template.
 */
Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate()
{
    return Local<ObjectTemplate>();
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
