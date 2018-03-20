//
//  Template.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

/**
 * Adds a property to each instance created by this template.
 *
 * The property must be defined either as a primitive value, or a template.
 */
void Template::Set(Local<Name> name, Local<Data> value,
         PropertyAttribute attributes)
{
    
}
void Template::SetPrivate(Local<Private> name, Local<Data> value,
                PropertyAttribute attributes)
{
    
}

void Template::SetAccessorProperty(
                         Local<Name> name,
                         Local<FunctionTemplate> getter,
                         Local<FunctionTemplate> setter,
                         PropertyAttribute attribute,
                         AccessControl settings)
{
    
}

/**
 * Whenever the property with the given name is accessed on objects
 * created from this Template the getter and setter callbacks
 * are called instead of getting and setting the property directly
 * on the JavaScript object.
 *
 * \param name The name of the property for which an accessor is added.
 * \param getter The callback to invoke when getting the property.
 * \param setter The callback to invoke when setting the property.
 * \param data A piece of data that will be passed to the getter and setter
 *   callbacks whenever they are invoked.
 * \param settings Access control settings for the accessor. This is a bit
 *   field consisting of one of more of
 *   DEFAULT = 0, ALL_CAN_READ = 1, or ALL_CAN_WRITE = 2.
 *   The default is to not allow cross-context access.
 *   ALL_CAN_READ means that all cross-context reads are allowed.
 *   ALL_CAN_WRITE means that all cross-context writes are allowed.
 *   The combination ALL_CAN_READ | ALL_CAN_WRITE can be used to allow all
 *   cross-context access.
 * \param attribute The attributes of the property for which an accessor
 *   is added.
 * \param signature The signature describes valid receivers for the accessor
 *   and is used to perform implicit instance checks against them. If the
 *   receiver is incompatible (i.e. is not an instance of the constructor as
 *   defined by FunctionTemplate::HasInstance()), an implicit TypeError is
 *   thrown and no callback is invoked.
 */
void Template::SetNativeDataProperty(
                           Local<String> name, AccessorGetterCallback getter,
                           AccessorSetterCallback setter,
                           // TODO(dcarney): gcc can't handle Local below
                           Local<Value> data, PropertyAttribute attribute,
                           Local<AccessorSignature> signature,
                           AccessControl settings)
{
    
}
void Template::SetNativeDataProperty(
                           Local<Name> name, AccessorNameGetterCallback getter,
                           AccessorNameSetterCallback setter,
                           // TODO(dcarney): gcc can't handle Local below
                           Local<Value> data, PropertyAttribute attribute,
                           Local<AccessorSignature> signature,
                           AccessControl settings)
{
    
}

/**
 * Like SetNativeDataProperty, but V8 will replace the native data property
 * with a real data property on first access.
 */
void Template::SetLazyDataProperty(Local<Name> name, AccessorNameGetterCallback getter,
                         Local<Value> data,
                         PropertyAttribute attribute)
{
    
}

/**
 * During template instantiation, sets the value with the intrinsic property
 * from the correct context.
 */
void Template::SetIntrinsicDataProperty(Local<Name> name, Intrinsic intrinsic,
                              PropertyAttribute attribute)
{
    
}

Local<Signature> Signature::New(Isolate* isolate,
                                Local<FunctionTemplate> receiver)
{
    return Local<Signature>();
}

Local<AccessorSignature> AccessorSignature::New(Isolate* isolate,
                                                Local<FunctionTemplate> receiver)
{
    return Local<AccessorSignature>();
}

