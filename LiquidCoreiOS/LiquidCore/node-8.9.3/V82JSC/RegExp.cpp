//
//  RegExp.cpp
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
 * Creates a regular expression from the given pattern string and
 * the flags bit field. May throw a JavaScript exception as
 * described in ECMA-262, 15.10.4.1.
 *
 * For example,
 *   RegExp::New(v8::String::New("foo"),
 *               static_cast<RegExp::Flags>(kGlobal | kMultiline))
 * is equivalent to evaluating "/foo/gm".
 */
MaybeLocal<RegExp> RegExp::New(Local<Context> context,
                              Local<String> pattern,
                              Flags flags)
{
    JSContextRef ctx = V82JSC::ToContextRef(context);
    
    char sflags[16] = {0};
    if (flags & Flags::kMultiline) strcat(sflags, "m");
    if (flags & Flags::kGlobal) strcat(sflags, "g");
    if (flags & Flags::kIgnoreCase) strcat(sflags, "i");
    if (flags & Flags::kUnicode) strcat(sflags, "u");
    if (flags & Flags::kSticky) strcat(sflags, "y");
    if (flags & Flags::kDotAll) strcat(sflags, "s");
    JSStringRef flagsref = JSStringCreateWithUTF8CString(sflags);
    
    JSValueRef args[] = {
        V82JSC::ToJSValueRef(pattern, context),
        JSValueMakeString(ctx, flagsref)
    };
    JSStringRelease(flagsref);
    LocalException exception(V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context)));
    JSObjectRef regexp = JSObjectMakeRegExp(ctx, 2, args, &exception);
    if (exception.ShouldThow()) {
        return MaybeLocal<RegExp>();
    }
    return ValueImpl::New(V82JSC::ToContextImpl(context), regexp).As<RegExp>();
}

/**
 * Returns the value of the source property: a string representing
 * the regular expression.
 */
Local<String> RegExp::GetSource() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(this);
    JSValueRef source = V82JSC::exec(ctx, "return _1.source", 1, &impl->m_value);
    return ValueImpl::New(V82JSC::ToContextImpl(context), source).As<String>();
}

/**
 * Returns the flags bit field.
 */
RegExp::Flags RegExp::GetFlags() const
{
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl>(this);
    JSValueRef flags = V82JSC::exec(ctx, "return _1.flags", 1, &impl->m_value);
    JSValueRef excp=0;
    char sflags[16];
    JSStringRef s = JSValueToStringCopy(ctx, flags, &excp);
    assert(excp==0);
    JSStringGetUTF8CString(s, sflags, 16);
    JSStringRelease(s);
    int flags_ = Flags::kNone;
    if (strchr(sflags,'m')) flags_ |= Flags::kMultiline;
    if (strchr(sflags,'g')) flags_ |= Flags::kGlobal;
    if (strchr(sflags,'i')) flags_ |= Flags::kIgnoreCase;
    if (strchr(sflags,'u')) flags_ |= Flags::kUnicode;
    if (strchr(sflags,'y')) flags_ |= Flags::kSticky;
    if (strchr(sflags,'s')) flags_ |= Flags::kDotAll;

    return static_cast<Flags>(flags_);
}
