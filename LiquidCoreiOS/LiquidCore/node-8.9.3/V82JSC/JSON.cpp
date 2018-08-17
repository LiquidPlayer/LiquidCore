//
//  JSON.cpp
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

