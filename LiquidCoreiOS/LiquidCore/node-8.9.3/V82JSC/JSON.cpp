/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

/**
 * Tries to parse the string |json_string| and returns it as value if
 * successful.
 *
 * \param json_string The string to parse.
 * \return The corresponding value if successfully parsed.
 */
MaybeLocal<Value> JSON::Parse(Local<Context> context, Local<String> json_string)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    JSValueRef string = V82JSC::ToJSValueRef(json_string, context);
    
    LocalException exception(V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context)));
    JSValueRef value = V82JSC::exec(ctx, "return JSON.parse(_1)", 1, &string, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<Value>();
    }
    return ValueImpl::New(V82JSC::ToContextImpl(context), value);
}

/**
 * Tries to stringify the JSON-serializable object |json_object| and returns
 * it as string if successful.
 *
 * \param json_object The JSON-serializable object to stringify.
 * \return The corresponding string if successfully stringified.
 */
MaybeLocal<String> JSON::Stringify(Local<Context> context, Local<Object> json_object,
                                   Local<String> gap)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(json_object, context),
        V82JSC::ToJSValueRef(gap, context)
    };
    
    LocalException exception(V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context)));
    JSValueRef value = V82JSC::exec(ctx, "return JSON.stringify(_1, null, _2)", 2, args, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<String>();
    }
    return ValueImpl::New(V82JSC::ToContextImpl(context), value).As<String>();
}

