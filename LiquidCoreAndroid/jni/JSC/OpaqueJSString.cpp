//
// OpaqueJSString.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

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

#include <string>
#include "JSC/OpaqueJSString.h"
#include "utf8.h"

JSStringRef OpaqueJSString::New(Local<String> string)
{
    return new OpaqueJSString(string);
}

JSStringRef OpaqueJSString::New(const JSChar * chars, size_t numChars)
{
    return new OpaqueJSString(chars, numChars);
}

JSStringRef OpaqueJSString::New(const char * chars)
{
    return new OpaqueJSString(chars);
}

OpaqueJSString::OpaqueJSString(Local<String> string) : m_isNull(false)
{
    String::Utf8Value chars(string);
    utf8::utf8to16(*chars, *chars + strlen(*chars), std::back_inserter(backstore));
}

OpaqueJSString::OpaqueJSString(const JSChar * chars, size_t numChars) :
    backstore(chars, chars + numChars) , m_isNull(!chars)
{
}

OpaqueJSString::OpaqueJSString(const char * chars) : m_isNull(!chars)
{
    if (chars) {
        utf8::utf8to16(chars, chars + strlen(chars), std::back_inserter(backstore));
    }
}

OpaqueJSString::~OpaqueJSString()
{
}

Local<String> OpaqueJSString::Value(Isolate *isolate)
{
    std::string utf8str;
    Utf8String(utf8str);
    return String::NewFromUtf8(isolate, utf8str.c_str());
}

const JSChar * OpaqueJSString::Chars()
{
    static const JSChar emptyString[] = {0};

    if (m_isNull) return nullptr;
    if (backstore.data() == nullptr) return emptyString;

    return backstore.data();
}

size_t OpaqueJSString::Size()
{
    return backstore.size();
}

size_t OpaqueJSString::Utf8Bytes()
{
    std::string utf8str;
    utf8::utf16to8(backstore.begin(), backstore.end(), std::back_inserter(utf8str));
    return utf8str.length() + 1;
}

void OpaqueJSString::Utf8String(std::string& utf8str)
{
    utf8::utf16to8(backstore.begin(), backstore.end(), std::back_inserter(utf8str));
}

bool OpaqueJSString::Equals(OpaqueJSString& other)
{
    return backstore == other.backstore;
}
