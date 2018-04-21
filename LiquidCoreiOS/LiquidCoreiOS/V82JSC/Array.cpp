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
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,Array>(this);
    JSValueRef length = V82JSC::exec(impl->m_context->m_context, "return _1.length", 1, &impl->m_value);
    uint32_t len = 0;
    if (length) {
        JSValueRef excp = 0;
        len = JSValueToNumber(impl->m_context->m_context, length, &excp);
        assert(excp==0);
    }
    return len;
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
