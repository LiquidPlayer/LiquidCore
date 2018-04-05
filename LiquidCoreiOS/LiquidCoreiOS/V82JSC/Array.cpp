//
//  Array.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

uint32_t Array::Length() const
{
    v8::internal::Object **map = reinterpret_cast<v8::internal::Object**>(const_cast<Array*>(this));
    v8::internal::Object *obj = *map;
    ValueImpl *impl = reinterpret_cast<ValueImpl*>(reinterpret_cast<intptr_t>(obj) & ~3);
    _local<Object> o((void*)this);
    _local<Context> context(impl->m_context);
    
    MaybeLocal<Value> length = (o.val_)->Get(context.toLocal(),
                                             String::NewFromUtf8(reinterpret_cast<Isolate*>(impl->m_context->isolate),
                                                                 "length", NewStringType::kNormal).ToLocalChecked());
    return JSValueToNumber(impl->m_context->m_context, reinterpret_cast<ValueImpl*>(*length.ToLocalChecked())->m_value, 0);
}

/**
 * Creates a JavaScript array with the given length. If the length
 * is negative the returned array will have length 0.
 */
Local<Array> Array::New(Isolate* isolate, int length)
{
    length = length<0 ? 0 : length;
    IsolateImpl *i = reinterpret_cast<IsolateImpl*>(isolate);
    JSValueRef args[length];
    for (int ndx=0; ndx < length; ndx++) {
        args[ndx] = JSValueMakeUndefined(i->m_defaultContext->m_context);
    }
    Local<Value> o = ValueImpl::New(i->m_defaultContext, JSObjectMakeArray(i->m_defaultContext->m_context, length, args, 0));
    _local<Array> obj(*o);
    return obj.toLocal();
}
