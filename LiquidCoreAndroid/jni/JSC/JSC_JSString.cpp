//
// JSC_JSString.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
