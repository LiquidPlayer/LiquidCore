/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSC/JSC.h"

JS_EXPORT JSStringRef JSStringCreateWithCharacters(const JSChar* chars, size_t numChars)
{
    return &* OpaqueJSString::New(chars, numChars);
}

JS_EXPORT JSStringRef JSStringCreateWithUTF8CString(const char* chars)
{
    return &* OpaqueJSString::New(chars);
}

JS_EXPORT JSStringRef JSStringRetain(JSStringRef string)
{
    string->retain();
    return string;
}

JS_EXPORT void JSStringRelease(JSStringRef string)
{
    string->release();
}

JS_EXPORT size_t JSStringGetLength(JSStringRef string)
{
    return string->Size();
}

JS_EXPORT const JSChar* JSStringGetCharactersPtr(JSStringRef string)
{
    return string->Chars();
}

JS_EXPORT size_t JSStringGetMaximumUTF8CStringSize(JSStringRef string)
{
    return string->Utf8Bytes();
}

JS_EXPORT size_t JSStringGetUTF8CString(JSStringRef string, char* buffer, size_t bufferSize)
{
    std::string utf8str;
    string->Utf8String(utf8str);
    strncpy(buffer, utf8str.c_str(), bufferSize);
    return strlen(buffer);
}

JS_EXPORT bool JSStringIsEqual(JSStringRef a, JSStringRef b)
{
    return a->Equals(*b);
}

JS_EXPORT bool JSStringIsEqualToUTF8CString(JSStringRef a, const char* b)
{
    OpaqueJSString b_(b);
    return JSStringIsEqual(a,&b_);
}
