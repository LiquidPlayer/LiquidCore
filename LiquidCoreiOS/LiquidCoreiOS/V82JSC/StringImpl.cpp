//
//  StringImpl.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "StringImpl.h"
#include "Value.h"

using namespace v8;

#undef THIS
#define THIS const_cast<StringImpl*>(static_cast<const StringImpl*>(this))

MaybeLocal<String> String::NewFromUtf8(Isolate* isolate, const char* data,
                                       v8::NewStringType type, int length)
{
    StringImpl * string = (StringImpl *) malloc(sizeof(StringImpl));
    memset(string, 0, sizeof(StringImpl));
    string->pInternal = (internal::String *)((reinterpret_cast<intptr_t>(string) & ~3) + 1);
    
    char str_[length>=0 ? length : 0];
    if (length>0) {
        strncpy(str_, data, length);
        str_[length-1] = 0;
        data = str_;
    }
    string->m_string = JSStringCreateWithUTF8CString(data);
    
    return MaybeLocal<String>(Local<String>(string));
}

String::Utf8Value::~Utf8Value()
{
    if (str_) {
        free(str_);
        str_ = nullptr;
    }
    length_ = 0;
}

String::Utf8Value::Utf8Value(Local<v8::Value> obj)
{
    ValueImpl *value = static_cast<ValueImpl *>(*obj);
    JSValueRef exception = nullptr;
    auto str = JSValueToStringCopy(value->m_context->m_context, value->m_value, &exception);
    if (exception) {
        str_ = nullptr;
    } else {
        length_ = (int) JSStringGetLength(str);
        str_ = (char *) malloc(length_ + 1);
        JSStringGetUTF8CString(str, str_, length_ + 1);
    }
}

String::Value::Value(Local<v8::Value> obj)
{
    
}
String::Value::~Value()
{
    
}


/**
 * Returns the number of characters (UTF-16 code units) in this string.
 */
int String::Length() const
{
    return (int) JSStringGetLength(THIS->m_string);
}

/**
 * Returns the number of bytes in the UTF-8 encoded
 * representation of this string.
 */
int String::Utf8Length() const
{
    return (int) JSStringGetMaximumUTF8CStringSize(THIS->m_string);
}

/**
 * Returns whether this string is known to contain only one byte data,
 * i.e. ISO-8859-1 code points.
 * Does not read the string.
 * False negatives are possible.
 */
bool String::IsOneByte() const
{
    return false;
}

/**
 * Returns whether this string contain only one byte data,
 * i.e. ISO-8859-1 code points.
 * Will read the entire string in some cases.
 */
bool String::ContainsOnlyOneByte() const
{
    return false;
}

/**
 * Write the contents of the string to an external buffer.
 * If no arguments are given, expects the buffer to be large
 * enough to hold the entire string and NULL terminator. Copies
 * the contents of the string and the NULL terminator into the
 * buffer.
 *
 * WriteUtf8 will not write partial UTF-8 sequences, preferring to stop
 * before the end of the buffer.
 *
 * Copies up to length characters into the output buffer.
 * Only null-terminates if there is enough space in the buffer.
 *
 * \param buffer The buffer into which the string will be copied.
 * \param start The starting position within the string at which
 * copying begins.
 * \param length The number of characters to copy from the string.  For
 *    WriteUtf8 the number of bytes in the buffer.
 * \param options Various options that might affect performance of this or
 *    subsequent operations.
 * \return The number of characters copied to the buffer excluding the null
 *    terminator.  For WriteUtf8: The number of bytes copied to the buffer
 *    including the null terminator (if written).
 */
// 16-bit character codes.
int String::Write(uint16_t* buffer,
          int start,
          int length,
          int options) const
{
    return 0;
}
// One byte characters.
int String::WriteOneByte(uint8_t* buffer,
                 int start,
                 int length,
                 int options) const
{
    return 0;
}
// UTF-8 encoded characters.
int String::WriteUtf8(char* buffer,
              int length,
              int* nchars_ref,
              int options) const
{
    return 0;
}

/**
 * Returns true if the string is external
 */
bool String::IsExternal() const
{
    return false;
}

/**
 * Returns true if the string is both external and one-byte.
 */
bool String::IsExternalOneByte() const
{
    return false;
}

/**
 * Get the ExternalOneByteStringResource for an external one-byte string.
 * Returns NULL if IsExternalOneByte() doesn't return true.
 */
const String::ExternalOneByteStringResource* String::GetExternalOneByteStringResource() const
{
    return nullptr;
}

/** Allocates a new string from Latin-1 data.  Only returns an empty value
 * when length > kMaxLength. **/
MaybeLocal<String> String::NewFromOneByte(Isolate* isolate, const uint8_t* data, v8::NewStringType type,
                                          int length)
{
    return MaybeLocal<String>();
}

/** Allocates a new string from UTF-16 data. Only returns an empty value when
 * length > kMaxLength. **/
MaybeLocal<String> String::NewFromTwoByte(Isolate* isolate, const uint16_t* data, v8::NewStringType type,
                                          int length)
{
    return MaybeLocal<String>();
}

/**
 * Creates a new string by concatenating the left and the right strings
 * passed in as parameters.
 */
Local<String> String::Concat(Local<String> left, Local<String> right)
{
    return Local<String>(nullptr);
}

/**
 * Creates a new external string using the data defined in the given
 * resource. When the external string is no longer live on V8's heap the
 * resource will be disposed by calling its Dispose method. The caller of
 * this function should not otherwise delete or modify the resource. Neither
 * should the underlying buffer be deallocated or modified except through the
 * destructor of the external string resource.
 */
MaybeLocal<String> String::NewExternalTwoByte(Isolate* isolate, String::ExternalStringResource* resource)
{
    return MaybeLocal<String>();
}

/**
 * Associate an external string resource with this string by transforming it
 * in place so that existing references to this string in the JavaScript heap
 * will use the external string resource. The external string resource's
 * character contents need to be equivalent to this string.
 * Returns true if the string has been changed to be an external string.
 * The string is not modified if the operation fails. See NewExternal for
 * information on the lifetime of the resource.
 */
bool String::MakeExternal(String::ExternalStringResource* resource)
{
    return false;
}

/**
 * Creates a new external string using the one-byte data defined in the given
 * resource. When the external string is no longer live on V8's heap the
 * resource will be disposed by calling its Dispose method. The caller of
 * this function should not otherwise delete or modify the resource. Neither
 * should the underlying buffer be deallocated or modified except through the
 * destructor of the external string resource.
 */
MaybeLocal<String> String::NewExternalOneByte(Isolate* isolate, ExternalOneByteStringResource* resource)
{
    return MaybeLocal<String>();
}

/**
 * Associate an external string resource with this string by transforming it
 * in place so that existing references to this string in the JavaScript heap
 * will use the external string resource. The external string resource's
 * character contents need to be equivalent to this string.
 * Returns true if the string has been changed to be an external string.
 * The string is not modified if the operation fails. See NewExternal for
 * information on the lifetime of the resource.
 */
bool String::MakeExternal(ExternalOneByteStringResource* resource)
{
    return false;
}

/**
 * Returns true if this string can be made external.
 */
bool String::CanMakeExternal()
{
    return false;
}

