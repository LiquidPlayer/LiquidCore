//
//  Date.cpp
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "V82JSC.h"

using namespace v8;

MaybeLocal<Value> Date::New(Local<Context> context, double time)
{
    auto c = V82JSC::ToContextRef(context);
    auto t = JSValueMakeNumber(c, time);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));

    LocalException exception(iso);
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
