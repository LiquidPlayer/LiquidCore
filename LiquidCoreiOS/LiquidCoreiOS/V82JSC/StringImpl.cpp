//
//  StringImpl.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

#undef THIS
#define THIS const_cast<StringImpl*>(reinterpret_cast<const StringImpl*>(this))

template <class T>
class _local {
public:
    T* val_;
    Local<T> toLocal() { return *(reinterpret_cast<Local<T> *>(this)); }
    _local(void *v) { val_ = reinterpret_cast<T*>(v); }
};

Local<String> StringImpl::New(JSStringRef str, v8::internal::InstanceType type, void *resource)
{
    StringImpl * string = (StringImpl *) malloc(sizeof(StringImpl));
    _local<String> local(string);
    memset(string, 0, sizeof(StringImpl));
    string->pMap = (v8::internal::Map *)((reinterpret_cast<intptr_t>(&string->map) & ~3) + 1);
    string->m_string = str;
    if (type == v8::internal::FIRST_NONSTRING_TYPE) {
        if (local.val_->ContainsOnlyOneByte()) {
            string->pMap->set_instance_type(v8::internal::ONE_BYTE_STRING_TYPE);
        } else {
            string->pMap->set_instance_type(v8::internal::STRING_TYPE);
        }
    } else {
        string->pMap->set_instance_type(type);
    }

    * reinterpret_cast<void**>(&string->map + internal::Internals::kStringResourceOffset) = resource;
    
    return local.toLocal();
}

static std::map<void*,JSStringRef> s_string_map;

MaybeLocal<String> String::NewFromUtf8(Isolate* isolate, const char* data,
                                       v8::NewStringType type, int length)
{
    char str_[length>=0 ? length : 0];
    if (length>0) {
        strncpy(str_, data, length);
        str_[length-1] = 0;
        data = str_;
    }
    return MaybeLocal<String>(StringImpl::New(JSStringCreateWithUTF8CString(data)));
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
    auto value = const_cast<ValueImpl*>(static_cast<const ValueImpl*>(*obj));
    JSValueRef exception = nullptr;
    JSStringRef s = JSValueToStringCopy(value->m_context->m_context, value->m_value, &exception);
    if (exception) {
        s = JSStringCreateWithUTF8CString("undefined");
    }
    str_ = const_cast<JSChar*>(JSStringGetCharactersPtr(s));
    length_ = (int) JSStringGetLength(s);
    s_string_map[this->str_] = s;
}
String::Value::~Value()
{
    JSStringRelease(s_string_map[str_]);
    s_string_map.erase(str_);
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
    return IsExternalOneByte();
}

/**
 * Returns whether this string contain only one byte data,
 * i.e. ISO-8859-1 code points.
 * Will read the entire string in some cases.
 */
bool String::ContainsOnlyOneByte() const
{
    size_t len = JSStringGetLength(THIS->m_string);
    const uint16_t *buffer = JSStringGetCharactersPtr(THIS->m_string);
    for (size_t i = 0; i < len; i++ ) {
        if (buffer[i] > 255) return false;
    }
    
    return true;
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
    const JSChar *str = JSStringGetCharactersPtr(THIS->m_string);
    size_t len = JSStringGetLength(THIS->m_string);
    str = &str[start];
    len -= start;
    len = length < len ? length : len;
    memcpy(buffer, str, sizeof(uint16_t) * len);
    return (int) len;
}
// One byte characters.
int String::WriteOneByte(uint8_t* buffer,
                 int start,
                 int length,
                 int options) const
{
    size_t len = JSStringGetMaximumUTF8CStringSize(THIS->m_string);
    char str[len+1];
    JSStringGetUTF8CString(THIS->m_string, str, len+1);
    len -= start;
    len = length < len ? length : len;
    memcpy(buffer, &str[start], sizeof(uint8_t) * len);
    return (int) len;
}
// UTF-8 encoded characters.
int String::WriteUtf8(char* buffer,
              int length,
              int* nchars_ref,
              int options) const
{
    size_t chars = JSStringGetUTF8CString(THIS->m_string, buffer, length);
    *nchars_ref = (int) chars;
    return (int) chars;
}

/**
 * Returns true if the string is external
 */
bool String::IsExternal() const
{
    typedef internal::Object O;
    typedef internal::Internals I;
    O* obj = *reinterpret_cast<O* const*>(this);
    int representation = (I::GetInstanceType(obj) & I::kFullStringRepresentationMask);
    return representation == I::kExternalOneByteRepresentationTag | representation==I::kExternalTwoByteRepresentationTag;
}

/**
 * Returns true if the string is both external and one-byte.
 */
bool String::IsExternalOneByte() const
{
    typedef internal::Object O;
    typedef internal::Internals I;
    O* obj = *reinterpret_cast<O* const*>(this);
    int representation = (I::GetInstanceType(obj) & I::kFullStringRepresentationMask);
    return representation == I::kExternalOneByteRepresentationTag;
}

/**
 * Get the ExternalOneByteStringResource for an external one-byte string.
 * Returns NULL if IsExternalOneByte() doesn't return true.
 */
const String::ExternalOneByteStringResource* String::GetExternalOneByteStringResource() const
{
    if (IsExternalOneByte()) {
        return  * reinterpret_cast<ExternalOneByteStringResource**>(&THIS->map + internal::Internals::kStringResourceOffset);
    }
    return nullptr;
}

/** Allocates a new string from Latin-1 data.  Only returns an empty value
 * when length > kMaxLength. **/
MaybeLocal<String> String::NewFromOneByte(Isolate* isolate, const uint8_t* data, v8::NewStringType type,
                                          int length)
{
    if (length < 0) {
        for (length = 0; data[length] != 0; length++);
    }
    uint16_t str[length];
    for (int i=0; i<length; i++) str[i] = data[i];
    return StringImpl::New(JSStringCreateWithCharacters(str, length), v8::internal::ONE_BYTE_STRING_TYPE);
}

/** Allocates a new string from UTF-16 data. Only returns an empty value when
 * length > kMaxLength. **/
MaybeLocal<String> String::NewFromTwoByte(Isolate* isolate, const uint16_t* data, v8::NewStringType type,
                                          int length)
{
    if (length < 0) {
        for (length = 0; data[length] != 0; length++);
    }
    return StringImpl::New(JSStringCreateWithCharacters(data, length));
}

/**
 * Creates a new string by concatenating the left and the right strings
 * passed in as parameters.
 */
Local<String> String::Concat(Local<String> left, Local<String> right)
{
    auto left_ = const_cast<StringImpl*>(reinterpret_cast<const StringImpl*>(*left));
    auto right_ = const_cast<StringImpl*>(reinterpret_cast<const StringImpl*>(*right));
    size_t length_left = JSStringGetLength(left_->m_string);
    size_t length_right = JSStringGetLength(right_->m_string);
    uint16_t concat[length_left + length_right];
    memcpy(concat, JSStringGetCharactersPtr(left_->m_string), sizeof(uint16_t) * length_left);
    memcpy(&concat[length_left], JSStringGetCharactersPtr(right_->m_string), sizeof(uint16_t) * length_right);
    return StringImpl::New(JSStringCreateWithCharacters(concat,length_left+length_right));
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
    if (resource->length() > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    return StringImpl::New(JSStringCreateWithCharacters(resource->data(), resource->length()),
                           v8::internal::EXTERNAL_STRING_TYPE, resource);
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
    if (THIS->m_string) {
        JSStringRelease(THIS->m_string);
    }
    THIS->m_string = JSStringCreateWithCharacters(resource->data(), resource->length());
    THIS->pMap->set_instance_type(v8::internal::EXTERNAL_STRING_TYPE);
    * reinterpret_cast<ExternalStringResource**>(&THIS->map + internal::Internals::kStringResourceOffset) = resource;
    return true;
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
    if (resource->length() > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    uint16_t str[resource->length()];
    for (int i=0; i<resource->length(); i++) str[i] = resource->data()[i];
    
    return StringImpl::New(JSStringCreateWithCharacters(str, resource->length()),
                           v8::internal::EXTERNAL_ONE_BYTE_STRING_TYPE, resource);
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
    uint16_t str[resource->length()];
    for (int i=0; i<resource->length(); i++) str[i] = resource->data()[i];
    if (THIS->m_string) {
        JSStringRelease(THIS->m_string);
    }
    THIS->m_string = JSStringCreateWithCharacters(str, resource->length());
    THIS->pMap->set_instance_type(v8::internal::EXTERNAL_ONE_BYTE_STRING_TYPE);
    * reinterpret_cast<ExternalOneByteStringResource**>(&THIS->map + internal::Internals::kStringResourceOffset) = resource;

    return true;
}

/**
 * Returns true if this string can be made external.
 */
bool String::CanMakeExternal()
{
    return true;
}

