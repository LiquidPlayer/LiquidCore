//
// Created by Eric on 11/6/16.
//

#include <string>
#include "JSC.h"
#include "utf8.h"

OpaqueJSString::OpaqueJSString(Local<String> string)
{
    String::Utf8Value chars(string);
    utf8::utf8to16(*chars, *chars + strlen(*chars), std::back_inserter(backstore));
}

OpaqueJSString::OpaqueJSString(const JSChar * chars, size_t numChars) :
    backstore(chars, chars + numChars)
{
}

OpaqueJSString::OpaqueJSString(const char * chars)
{
    utf8::utf8to16(chars, chars + strlen(chars), std::back_inserter(backstore));
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
    return utf8str.length();
}

void OpaqueJSString::Utf8String(std::string& utf8str)
{
    utf8::utf16to8(backstore.begin(), backstore.end(), std::back_inserter(utf8str));
}

bool OpaqueJSString::Equals(OpaqueJSString& other)
{
    return backstore == other.backstore;
}

JS_EXPORT JSStringRef JSStringCreateWithCharacters(const JSChar* chars, size_t numChars)
{
    return new OpaqueJSString(chars, numChars);
}

JS_EXPORT JSStringRef JSStringCreateWithUTF8CString(const char* chars)
{
    return new OpaqueJSString(chars);
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
