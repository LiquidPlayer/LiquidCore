/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace V82JSC;
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
    EscapableHandleScope scope(ToIsolate(ToContextImpl(context)));
    JSContextRef ctx = ToContextRef(context);
    
    char sflags[16] = {0};
    if (flags & Flags::kMultiline) strcat(sflags, "m");
    if (flags & Flags::kGlobal) strcat(sflags, "g");
    if (flags & Flags::kIgnoreCase) strcat(sflags, "i");
    if (flags & Flags::kUnicode) strcat(sflags, "u");
    if (flags & Flags::kSticky) strcat(sflags, "y");
    if (flags & Flags::kDotAll) strcat(sflags, "s");
    JSStringRef flagsref = JSStringCreateWithUTF8CString(sflags);
    
    JSValueRef args[] = {
        ToJSValueRef(pattern, context),
        JSValueMakeString(ctx, flagsref)
    };
    JSStringRelease(flagsref);
    LocalException exception(ToIsolateImpl(ToContextImpl(context)));
    JSObjectRef regexp = JSObjectMakeRegExp(ctx, 2, args, &exception);
    if (exception.ShouldThrow()) {
        return MaybeLocal<RegExp>();
    }
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), regexp).As<RegExp>());
}

/**
 * Returns the value of the source property: a string representing
 * the regular expression.
 */
Local<v8::String> RegExp::GetSource() const
{
    EscapableHandleScope scope(ToIsolate(this));
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    auto impl = ToImpl<V82JSC::Value>(this);
    JSValueRef source = exec(ctx, "return _1.source", 1, &impl->m_value);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), source).As<String>());
}

/**
 * Returns the flags bit field.
 */
RegExp::Flags RegExp::GetFlags() const
{
    HandleScope scope(ToIsolate(this));
    Local<Context> context = ToCurrentContext(this);
    JSContextRef ctx = ToContextRef(context);
    auto impl = ToImpl<V82JSC::Value>(this);
    JSValueRef flags = exec(ctx, "return _1.flags", 1, &impl->m_value);
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
