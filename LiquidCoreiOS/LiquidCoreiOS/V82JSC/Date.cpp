//
//  Date.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

MaybeLocal<Value> Date::New(Local<Context> context, double time)
{
    auto c = V82JSC::ToContextRef(context);
    auto t = JSValueMakeNumber(c, time);
    LocalException exception(V82JSC::ToContextImpl(context)->m_isolate);
    auto r = JSObjectMakeDate(c, 1, &t, &exception);
    if (!exception.ShouldThow()) {
        return ValueImpl::New(V82JSC::ToContextImpl(context), r);
    }
    return MaybeLocal<Value>();
}

/**
 * A specialization of Value::NumberValue that is more efficient
 * because we know the structure of this object.
 */
double Date::ValueOf() const
{
    return reinterpret_cast<const NumberObject*>(this)->ValueOf();
}

/**
 * Notification that the embedder has changed the time zone,
 * daylight savings time, or other date / time configuration
 * parameters.  V8 keeps a cache of various values used for
 * date / time computation.  This notification will reset
 * those cached values for the current context so that date /
 * time configuration changes would be reflected in the Date
 * object.
 *
 * This API should not be called more than needed as it will
 * negatively impact the performance of date operations.
 */
void Date::DateTimeConfigurationChangeNotification(Isolate* isolate)
{
    assert(0);
}
