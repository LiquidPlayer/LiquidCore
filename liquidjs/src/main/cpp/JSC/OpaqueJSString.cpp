/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
    String::Utf8Value chars(Isolate::GetCurrent(), string);
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
