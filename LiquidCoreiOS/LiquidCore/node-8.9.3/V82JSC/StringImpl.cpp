/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include "JSWeakRefPrivate.h"
#include "JSStringRefPrivate.h"
#include <codecvt>
#include "utf8.h"

using namespace v8;

#define H V82JSC_HeapObject

Local<v8::String> StringImpl::New(Isolate *isolate, JSStringRef str, H::BaseMap* type,
                                  void *resource, v8::NewStringType stringtype)
{
    EscapableHandleScope scope(isolate);
    IsolateImpl *i = V82JSC::ToIsolateImpl(isolate);
    Local<Context> context = i->m_nullContext.Get(isolate);
    JSContextRef ctx = V82JSC::ToContextRef(context);
    Context::Scope context_scope(context);

    StringImpl *string = static_cast<StringImpl *>(H::HeapAllocator::Alloc(V82JSC::ToIsolateImpl(isolate),
                                                                         type ? type : i->m_string_map));
    Local<v8::String> local = V82JSC::CreateLocal<v8::String>(isolate, string);
    string->m_value = JSValueMakeString(ctx, str);
    JSValueProtect(ctx, string->m_value);

    if (type == nullptr) {
        if (local->ContainsOnlyOneByte()) {
            type = i->m_one_byte_string_map;
        } else {
            type = i->m_string_map;
        }
        string->m_map = H::ToV8Map(type);
    }
    
    if (stringtype == NewStringType::kInternalized) {
        for (auto it=i->m_internalized_strings.begin(); it!=i->m_internalized_strings.end(); ++it) {
            if (JSStringIsEqual(str, it->first)) {
                JSStringRelease(str);
                return scope.Escape(it->second->Get(V82JSC::ToIsolate(i)));
            }
        }
        auto weak = new Copyable(v8::String)(V82JSC::ToIsolate(i), local);
        weak->SetWeak<Copyable(v8::String)>(weak, [](const WeakCallbackInfo<Copyable(v8::String)>& data) {
            IsolateImpl* iso = V82JSC::ToIsolateImpl(data.GetIsolate());
            auto weak = data.GetParameter();
            for (auto i=iso->m_internalized_strings.begin(); i!=iso->m_internalized_strings.end(); ) {
                if (i->second == weak) {
                    iso->m_internalized_strings.erase(i);
                    break;
                }
                i++;
            }
            weak->Reset();
            delete weak;
        }, v8::WeakCallbackType::kParameter);
        i->m_internalized_strings[str] = weak;
    }
    
    if ((type == i->m_external_string_map ||
         type == i->m_external_one_byte_string_map) &&
        resource != nullptr) {
        
        type = i->m_external_string_map ? i->m_weak_external_string_map : i->m_weak_external_one_byte_string_map;
        
        WeakExternalStringImpl::Init(i, string->m_value, (v8::String::ExternalStringResourceBase*) resource, type);
    }
    
    * reinterpret_cast<void**>(reinterpret_cast<intptr_t>(string) +
                               internal::Internals::kStringResourceOffset) = resource;

    if (stringtype == NewStringType::kNormal) {
        JSStringRelease(str);
    }
    
    return scope.Escape(local);
}

static std::map<void*,JSStringRef> s_string_map;

MaybeLocal<String> String::NewFromUtf8(Isolate* isolate, const char* data,
                                       v8::NewStringType type, int length)
{
    EscapableHandleScope scope(isolate);
    
    if (length > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    
    if (length < 0) {
        length = strlen(data);
    }
    
    std::vector<unsigned short> backstore;
    utf8::unchecked::utf8to16(data, data + length, std::back_inserter(backstore));

    // FIXME: Would be nice to use JSStringCreateWithCharactersNoCopy
    JSStringRef s = JSStringCreateWithCharacters(&backstore [0], length);
    Local<String> out = StringImpl::New(isolate, s, nullptr, nullptr, type);
    
    return scope.Escape(out);
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
    if (obj.IsEmpty()) {
        str_ = nullptr;
        length_ = 0;
    } else {
        HandleScope scope(Isolate::GetCurrent());
        Local<Context> context = V82JSC::OperatingContext(Isolate::GetCurrent());
        JSValueRef value = V82JSC::ToJSValueRef(obj, context);

        JSValueRef exception = nullptr;
        auto str = JSValueToStringCopy(V82JSC::ToContextRef(context), value, &exception);
        if (exception) {
            str_ = nullptr;
        } else {
            length_ = (int) JSStringGetMaximumUTF8CStringSize(str);
            str_ = (char *) malloc(length_);
            JSStringGetUTF8CString(str, str_, length_);
            JSStringRelease(str);
        }
    }
}

String::Value::Value(Local<v8::Value> obj)
{
    HandleScope scope(Isolate::GetCurrent());
    Local<Context> context = V82JSC::ToCurrentContext(*obj);
    JSValueRef value = V82JSC::ToJSValueRef(obj, context);
    
    JSValueRef exception = nullptr;
    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, &exception);
    if (exception) {
        s = JSStringCreateWithUTF8CString("undefined");
    }
    length_ = (int) JSStringGetLength(s);
    str_ = (JSChar*) malloc(sizeof(JSChar) * length_);
    memcpy (str_, JSStringGetCharactersPtr(s), sizeof(JSChar) * length_);
    JSStringRelease(s);
}
String::Value::~Value()
{
    if (str_) free(str_);
}

/**
 * Returns the number of characters (UTF-16 code units) in this string.
 */
int String::Length() const
{
    HandleScope scope(Isolate::GetCurrent());
    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);

    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, 0);
    int r = (int) JSStringGetLength(s);
    JSStringRelease(s);
    return r;
}

/**
 * Returns the number of bytes in the UTF-8 encoded
 * representation of this string.
 */
int String::Utf8Length() const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, 0);
    const JSChar * chars = JSStringGetCharactersPtr(s);
    int len = (int)JSStringGetLength(s);
    
    int c = 0;
    int i;
    for (i=0; i<len; ) {
        uint32_t unicode = (chars[i] >= 0xd800) ? (chars[i] - 0xd800) * 0x400 + (chars[i+1] - 0xdc00) : chars[i];
        i += (chars[i] >= 0xd800) ? 2 : 1;
        c += unicode >= 0x10000 ? 4 : unicode >= 0x800 ? 3 : unicode >= 0x80 ? 2 : 1;
    }
    JSStringRelease(s);
    return c - (i-len);
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
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, 0);
    if (!s) return false;

    size_t len = JSStringGetLength(s);
    const uint16_t *buffer = JSStringGetCharactersPtr(s);
    for (size_t i = 0; i < len; i++ ) {
        if (buffer[i] > 255) return false;
    }
    JSStringRelease(s);
    
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
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, 0);

    const JSChar *str = JSStringGetCharactersPtr(s);
    size_t len = JSStringGetLength(s);
    str = &str[start];
    len -= start;
    len = length < len ? length : len;
    memcpy(buffer, str, sizeof(uint16_t) * len);
    
    JSStringRelease(s);
    return (int) len;
}
// One byte characters.
int String::WriteOneByte(uint8_t* buffer,
                 int start,
                 int length,
                 int options) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, 0);

    size_t len = JSStringGetMaximumUTF8CStringSize(s);
    char str[len];
    JSStringGetUTF8CString(s, str, len);
    len -= start;
    len = length < len ? length : len;
    memcpy(buffer, &str[start], sizeof(uint8_t) * len);
    
    JSStringRelease(s);
    return (int) len;
}
// UTF-8 encoded characters.
int String::WriteUtf8(char* buffer,
              int length,
              int* nchars_ref,
              int options) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = V82JSC::ToCurrentContext(this);
    JSValueRef value = V82JSC::ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(V82JSC::ToContextRef(context), value, 0);

    if (length < 0) {
        length = (int) JSStringGetMaximumUTF8CStringSize(s);
    }
    // FIXME: This is an annoying inefficiency.  JSC needs the null-terminator to be
    // part of buffer length, but V8 does not.  So we allocate one additional byte and
    // then copy back the correct number.
    char temp[length + 1];
    size_t chars = JSStringGetUTF8CString(s, temp, length+1);
    memcpy(buffer, temp, length);
    if (nchars_ref) {
        *nchars_ref = (int) JSStringGetLength(s);
    }
    JSStringRelease(s);
    return (int) chars - 1;
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
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,String>(this);

    if (IsExternalOneByte()) {
        return  * reinterpret_cast<ExternalOneByteStringResource**>(reinterpret_cast<intptr_t>(impl) +
                                                                    internal::Internals::kStringResourceOffset);
    }
    return nullptr;
}

/** Allocates a new string from Latin-1 data.  Only returns an empty value
 * when length > kMaxLength. **/
MaybeLocal<String> String::NewFromOneByte(Isolate* isolate, const uint8_t* data, v8::NewStringType type,
                                          int length)
{
    EscapableHandleScope scope(isolate);

    if (length > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    if (length < 0) {
        for (length = 0; data[length] != 0; length++);
    }
    uint16_t str[length];
    for (int i=0; i<length; i++) str[i] = data[i];
    return scope.Escape(StringImpl::New(isolate, JSStringCreateWithCharacters(str, length),
                           V82JSC::ToIsolateImpl(isolate)->m_one_byte_string_map, nullptr, type));
}

/** Allocates a new string from UTF-16 data. Only returns an empty value when
 * length > kMaxLength. **/
MaybeLocal<String> String::NewFromTwoByte(Isolate* isolate, const uint16_t* data, v8::NewStringType type,
                                          int length)
{
    EscapableHandleScope scope(isolate);
    
    if (length > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    if (length < 0) {
        for (length = 0; data[length] != 0; length++);
    }
    return scope.Escape(StringImpl::New(isolate, JSStringCreateWithCharacters(data, length), nullptr, nullptr, type));
}

/**
 * Creates a new string by concatenating the left and the right strings
 * passed in as parameters.
 */
Local<String> String::Concat(Local<String> left, Local<String> right)
{
    EscapableHandleScope scope(Isolate::GetCurrent());

    if (left->Length() + right->Length() > v8::String::kMaxLength) {
        return Local<String>();
    }
    Local<Context> context = V82JSC::ToCurrentContext(*left);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(V82JSC::ToContextImpl(context));
    JSValueRef left_ = V82JSC::ToJSValueRef(left, context);
    JSValueRef right_ = V82JSC::ToJSValueRef(right, context);

    JSStringRef sleft = JSValueToStringCopy(V82JSC::ToContextRef(context), left_, 0);
    JSStringRef sright = JSValueToStringCopy(V82JSC::ToContextRef(context), right_, 0);

    size_t length_left = JSStringGetLength(sleft);
    size_t length_right = JSStringGetLength(sright);
    uint16_t concat[length_left + length_right];
    memcpy(concat, JSStringGetCharactersPtr(sleft), sizeof(uint16_t) * length_left);
    memcpy(&concat[length_left], JSStringGetCharactersPtr(sright), sizeof(uint16_t) * length_right);
    Isolate *isolate = V82JSC::ToIsolate(iso);
    JSStringRef concatted = JSStringCreateWithCharacters(concat,length_left+length_right);
    Local<String> ret = StringImpl::New(isolate, concatted);
    JSStringRelease(sleft);
    JSStringRelease(sright);
    JSStringRelease(concatted);
    return scope.Escape(ret);
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
    EscapableHandleScope scope(isolate);
    
    if (resource->length() > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    return scope.Escape(StringImpl::New(isolate, JSStringCreateWithCharactersNoCopy(resource->data(), resource->length()),
                           V82JSC::ToIsolateImpl(isolate)->m_external_string_map, resource));
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
    // NOTE: Making existing internalized strings external doesn't really do anything
    // other than report back when the string is collected.  The externalized resource
    // is not actually used by JSC.
    if (resource->length() > v8::String::kMaxLength) {
        return false;
    }
    ValueImpl *impl = V82JSC::ToImpl<ValueImpl,String>(this);
    IsolateImpl *iso = V82JSC::ToIsolateImpl(impl);
    impl->m_map = H::ToV8Map(iso->m_external_string_map);

    WeakExternalStringImpl::Init(iso, impl->m_value, resource, iso->m_weak_external_string_map);

    * reinterpret_cast<ExternalStringResource**>(reinterpret_cast<intptr_t>(impl) +
                               internal::Internals::kStringResourceOffset) = resource;

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
    EscapableHandleScope scope(isolate);
    
    if (resource->length() > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    uint16_t *str = (uint16_t*) calloc(sizeof (uint16_t), resource->length());
    for (int i=0; i<resource->length(); i++) str[i] = resource->data()[i];

    Local<String> s = StringImpl::New(isolate, JSStringCreateWithCharacters(str, resource->length()),
                                      V82JSC::ToIsolateImpl(isolate)->m_external_one_byte_string_map, resource);
    delete str;
    return scope.Escape(s);
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
    // NOTE: Making existing internalized strings external doesn't really do anything
    // other than report back when the string is collected.  The externalized resource
    // is not actually used by JSC.
    if (resource->length() > v8::String::kMaxLength) {
        return false;
    }
    StringImpl *impl = V82JSC::ToImpl<StringImpl,String>(this);
    IsolateImpl *iso = V82JSC::ToIsolateImpl(impl);
    impl->m_map = H::ToV8Map(iso->m_external_one_byte_string_map);

    WeakExternalStringImpl::Init(iso, impl->m_value, resource, iso->m_weak_external_one_byte_string_map);
    
    * reinterpret_cast<ExternalOneByteStringResource**>(reinterpret_cast<intptr_t>(impl) +
                                                 internal::Internals::kStringResourceOffset) = resource;

    return true;
}

/**
 * Returns true if this string can be made external.
 */
bool String::CanMakeExternal()
{
    return true;
}

void WeakExternalStringImpl::Init(IsolateImpl* iso, JSValueRef value, String::ExternalStringResourceBase *resource,
                                  V82JSC_HeapObject::BaseMap *map)
{
    HandleScope scope(V82JSC::ToIsolate(iso));
    WeakExternalStringImpl *ext = static_cast<WeakExternalStringImpl*>(H::HeapAllocator::Alloc(iso, map));
    ext->m_value = value;
    ext->m_weakRef = JSWeakCreate(iso->m_group, (JSObjectRef)value);
    ext->m_resource = resource;
    assert(JSWeakGetObject(ext->m_weakRef) == (JSObjectRef)value);
    Local<v8::WeakExternalString> wes = V82JSC::CreateLocal<v8::WeakExternalString>(&iso->ii, ext);
    iso->m_external_strings[ext->m_value] = Copyable(v8::WeakExternalString)(V82JSC::ToIsolate(iso), wes);
}

