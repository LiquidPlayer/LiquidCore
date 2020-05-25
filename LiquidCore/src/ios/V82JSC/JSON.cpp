/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
using v8::MaybeLocal;
using v8::Local;
using v8::JSON;

/**
 * Tries to parse the string |json_string| and returns it as value if
 * successful.
 *
 * \param json_string The string to parse.
 * \return The corresponding value if successfully parsed.
 */
MaybeLocal<v8::Value> JSON::Parse(Local<Context> context, Local<String> json_string)
{
    EscapableHandleScope scope(ToIsolate(ToContextImpl(context)));
    JSContextRef ctx = ToContextRef(context);
    JSValueRef string = ToJSValueRef(json_string, context);
    
    LocalException exception(ToIsolateImpl(ToContextImpl(context)));
    JSValueRef value = exec(ctx, "return JSON.parse(_1)", 1, &string, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<Value>();
    }
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), value));
}

/**
 * Tries to stringify the JSON-serializable object |json_object| and returns
 * it as string if successful.
 *
 * \param json_object The JSON-serializable object to stringify.
 * \return The corresponding string if successfully stringified.
 */
MaybeLocal<v8::String> JSON::Stringify(Local<Context> context, Local<Value> json_object,
                                       Local<String> gap)
{
    EscapableHandleScope scope(ToIsolate(ToContextImpl(context)));
    JSContextRef ctx = ToContextRef(context);
    
    JSValueRef args[] = {
        ToJSValueRef(json_object, context),
        ToJSValueRef(gap, context)
    };
    
    LocalException exception(ToIsolateImpl(ToContextImpl(context)));
    JSValueRef value = exec(ctx, "return JSON.stringify(_1, null, _2)", 2, args, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<String>();
    }
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), value).As<String>());
}

