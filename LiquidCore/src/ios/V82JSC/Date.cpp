/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using v8::Date;
using v8::MaybeLocal;
using v8::Local;

MaybeLocal<v8::Value> Date::New(Local<v8::Context> context, double time)
{
    auto c = ToContextRef(context);
    auto t = JSValueMakeNumber(c, time);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));

    LocalException exception(iso);
    auto r = JSObjectMakeDate(c, 1, &t, &exception);
    if (!exception.ShouldThrow()) {
        return V82JSC::Value::New(ToContextImpl(context), r);
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
    NOT_IMPLEMENTED;
}
