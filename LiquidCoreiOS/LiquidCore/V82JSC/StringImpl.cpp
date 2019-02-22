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
#include "StringImpl.h"

using namespace V82JSC;
using namespace v8;

Local<v8::String> V82JSC::String::New(Isolate *isolate, JSStringRef str, BaseMap* type,
                                  void *resource, v8::NewStringType stringtype)
{
    EscapableHandleScope scope(isolate);
    IsolateImpl *i = ToIsolateImpl(isolate);
    Local<v8::Context> context = i->m_nullContext.Get(isolate);
    JSContextRef ctx = ToContextRef(context);
    v8::Context::Scope context_scope(context);

    auto string = static_cast<V82JSC::String *>(HeapAllocator::Alloc(ToIsolateImpl(isolate),
                                                                             type ? type : i->m_string_map));
    Local<v8::String> local = CreateLocal<v8::String>(isolate, string);
    string->m_value = JSValueMakeString(ctx, str);
    JSValueProtect(ctx, string->m_value);

    if (type == nullptr) {
        if (local->ContainsOnlyOneByte()) {
            type = i->m_one_byte_string_map;
        } else {
            type = i->m_string_map;
        }
        string->m_map = ToV8Map(type);
    }
    
    if (stringtype == NewStringType::kInternalized) {
        for (auto it=i->m_internalized_strings.begin(); it!=i->m_internalized_strings.end(); ++it) {
            if (JSStringIsEqual(str, it->first)) {
                JSStringRelease(str);
                return scope.Escape(it->second->Get(ToIsolate(i)));
            }
        }
        auto weak = new v8::Persistent<v8::String>(ToIsolate(i), local);
        weak->SetWeak<v8::Persistent<v8::String>>(weak, [](const WeakCallbackInfo<v8::Persistent<v8::String>>& data) {
            IsolateImpl* iso = ToIsolateImpl(data.GetIsolate());
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
        
        WeakExternalString::Init(i, string->m_value, (v8::String::ExternalStringResourceBase*) resource, type);
    }
    
    * reinterpret_cast<void**>(reinterpret_cast<intptr_t>(string) +
                               internal::Internals::kStringResourceOffset) = resource;

    if (stringtype == NewStringType::kNormal) {
        JSStringRelease(str);
    }
    
    return scope.Escape(local);
}

void V82JSC::WeakExternalString::FinalizeExternalString()
{
    v8::internal::Isolate *ii = reinterpret_cast<v8::internal::Isolate*>(GetIsolate());
    if (m_resource) {
        v8::String::ExternalStringResourceBase *rsrc = m_resource;
        int count = 0;
        std::map<JSValueRef, v8::Persistent<v8::WeakExternalString>>::iterator todelete;
        for(auto i=GetIsolate()->m_external_strings.begin(); i!=GetIsolate()->m_external_strings.end(); ++i) {
            auto s = i->second.Get(reinterpret_cast<v8::Isolate*>(GetIsolate()));
            WeakExternalString* wes = static_cast<WeakExternalString*>(FromHeapPointer(*(internal::Object**)*s));
            if (wes->m_resource == rsrc && wes != this) count++;
        }
        if (count == 0) {
            intptr_t addr = reinterpret_cast<intptr_t>(this) + v8::internal::ExternalString::kResourceOffset;
            * reinterpret_cast<v8::String::ExternalStringResourceBase**>(addr) = rsrc;
            ii->heap()->FinalizeExternalString(reinterpret_cast<v8::internal::String*>(ToHeapPointer(this)));
        }
    }
}

static std::map<void*,JSStringRef> s_string_map;

MaybeLocal<v8::String> v8::String::NewFromUtf8(Isolate* isolate, const char* data,
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
    utf8::utf8to16(data, data + length, std::back_inserter(backstore));

    // FIXME: Would be nice to use JSStringCreateWithCharactersNoCopy
    JSStringRef s = JSStringCreateWithCharacters(&backstore [0], backstore.size());
    Local<String> out = V82JSC::String::New(isolate, s, nullptr, nullptr, type);
    
    return scope.Escape(out);
}

v8::String::Utf8Value::~Utf8Value()
{
    if (str_) {
        free(str_);
        str_ = nullptr;
    }
    length_ = 0;
}

v8::String::Utf8Value::Utf8Value(Local<v8::Value> obj)
{
    if (obj.IsEmpty()) {
        str_ = nullptr;
        length_ = 0;
    } else {
        HandleScope scope(Isolate::GetCurrent());
        Local<Context> context = OperatingContext(Isolate::GetCurrent());
        JSValueRef value = ToJSValueRef(obj, context);

        JSValueRef exception = nullptr;
        auto str = JSValueToStringCopy(ToContextRef(context), value, &exception);
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

v8::String::Value::Value(Local<v8::Value> obj)
{
    HandleScope scope(Isolate::GetCurrent());
    Local<Context> context = ToCurrentContext(*obj);
    JSValueRef value = ToJSValueRef(obj, context);
    
    JSValueRef exception = nullptr;
    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, &exception);
    if (exception) {
        s = JSStringCreateWithUTF8CString("undefined");
    }
    length_ = (int) JSStringGetLength(s);
    str_ = (JSChar*) malloc(sizeof(JSChar) * length_);
    memcpy (str_, JSStringGetCharactersPtr(s), sizeof(JSChar) * length_);
    JSStringRelease(s);
}
v8::String::Value::~Value()
{
    if (str_) free(str_);
}

/**
 * Returns the number of characters (UTF-16 code units) in this string.
 */
int v8::String::Length() const
{
    HandleScope scope(Isolate::GetCurrent());
    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);

    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, 0);
    int r = (int) JSStringGetLength(s);
    JSStringRelease(s);
    return r;
}

/**
 * Returns the number of bytes in the UTF-8 encoded
 * representation of this string.
 */
int v8::String::Utf8Length() const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, 0);
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
bool v8::String::IsOneByte() const
{
    return IsExternalOneByte();
}

/**
 * Returns whether this string contain only one byte data,
 * i.e. ISO-8859-1 code points.
 * Will read the entire string in some cases.
 */
bool v8::String::ContainsOnlyOneByte() const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, 0);
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
int v8::String::Write(uint16_t* buffer,
          int start,
          int length,
          int options) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, 0);

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
int v8::String::WriteOneByte(uint8_t* buffer,
                 int start,
                 int length,
                 int options) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, 0);

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
int v8::String::WriteUtf8(char* buffer,
              int length,
              int* nchars_ref,
              int options) const
{
    HandleScope scope(Isolate::GetCurrent());

    Local<Context> context = ToCurrentContext(this);
    JSValueRef value = ToJSValueRef(this, context);
    JSStringRef s = JSValueToStringCopy(ToContextRef(context), value, 0);

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
bool v8::String::IsExternal() const
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
bool v8::String::IsExternalOneByte() const
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
const v8::String::ExternalOneByteStringResource* v8::String::GetExternalOneByteStringResource() const
{
    auto impl = ToImpl<Value,String>(this);

    if (IsExternalOneByte()) {
        return  * reinterpret_cast<ExternalOneByteStringResource**>(reinterpret_cast<intptr_t>(impl) +
                                                                    internal::Internals::kStringResourceOffset);
    }
    return nullptr;
}

/** Allocates a new string from Latin-1 data.  Only returns an empty value
 * when length > kMaxLength. **/
MaybeLocal<v8::String> v8::String::NewFromOneByte(Isolate* isolate, const uint8_t* data, v8::NewStringType type,
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
    return scope.Escape(V82JSC::String::New(isolate, JSStringCreateWithCharacters(str, length),
                           ToIsolateImpl(isolate)->m_one_byte_string_map, nullptr, type));
}

/** Allocates a new string from UTF-16 data. Only returns an empty value when
 * length > kMaxLength. **/
MaybeLocal<v8::String> v8::String::NewFromTwoByte(Isolate* isolate, const uint16_t* data, v8::NewStringType type,
                                          int length)
{
    EscapableHandleScope scope(isolate);
    
    if (length > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    if (length < 0) {
        for (length = 0; data[length] != 0; length++);
    }
    return scope.Escape(V82JSC::String::New(isolate, JSStringCreateWithCharacters(data, length), nullptr, nullptr, type));
}

/**
 * Creates a new string by concatenating the left and the right strings
 * passed in as parameters.
 */
Local<v8::String> v8::String::Concat(Local<String> left, Local<String> right)
{
    EscapableHandleScope scope(Isolate::GetCurrent());

    if (left->Length() + right->Length() > v8::String::kMaxLength) {
        return Local<String>();
    }
    Local<Context> context = ToCurrentContext(*left);
    IsolateImpl* iso = ToIsolateImpl(ToContextImpl(context));
    JSValueRef left_ = ToJSValueRef(left, context);
    JSValueRef right_ = ToJSValueRef(right, context);

    JSStringRef sleft = JSValueToStringCopy(ToContextRef(context), left_, 0);
    JSStringRef sright = JSValueToStringCopy(ToContextRef(context), right_, 0);

    size_t length_left = JSStringGetLength(sleft);
    size_t length_right = JSStringGetLength(sright);
    uint16_t concat[length_left + length_right];
    memcpy(concat, JSStringGetCharactersPtr(sleft), sizeof(uint16_t) * length_left);
    memcpy(&concat[length_left], JSStringGetCharactersPtr(sright), sizeof(uint16_t) * length_right);
    Isolate *isolate = ToIsolate(iso);
    JSStringRef concatted = JSStringCreateWithCharacters(concat,length_left+length_right);
    Local<String> ret = V82JSC::String::New(isolate, concatted);
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
MaybeLocal<v8::String> v8::String::NewExternalTwoByte(Isolate* isolate, String::ExternalStringResource* resource)
{
    EscapableHandleScope scope(isolate);
    
    if (resource->length() > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    return scope.Escape(V82JSC::String::New(isolate,
            JSStringCreateWithCharactersNoCopy(resource->data(), resource->length()),
            ToIsolateImpl(isolate)->m_external_string_map, resource));
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
bool v8::String::MakeExternal(v8::String::ExternalStringResource* resource)
{
    // NOTE: Making existing internalized strings external doesn't really do anything
    // other than report back when the string is collected.  The externalized resource
    // is not actually used by JSC.
    if (resource->length() > v8::String::kMaxLength) {
        return false;
    }
    auto impl = ToImpl<V82JSC::Value,String>(this);
    auto iso = ToIsolateImpl(impl);
    impl->m_map = ToV8Map(iso->m_external_string_map);

    V82JSC::WeakExternalString::Init(iso, impl->m_value, resource, iso->m_weak_external_string_map);

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
MaybeLocal<v8::String> v8::String::NewExternalOneByte(Isolate* isolate, ExternalOneByteStringResource* resource)
{
    EscapableHandleScope scope(isolate);
    
    if (resource->length() > v8::String::kMaxLength) {
        return MaybeLocal<String>();
    }
    uint16_t *str = (uint16_t*) calloc(sizeof (uint16_t), resource->length());
    for (int i=0; i<resource->length(); i++) str[i] = resource->data()[i];

    Local<String> s = V82JSC::String::New(isolate, JSStringCreateWithCharacters(str, resource->length()),
                                      ToIsolateImpl(isolate)->m_external_one_byte_string_map, resource);
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
bool v8::String::MakeExternal(ExternalOneByteStringResource* resource)
{
    // NOTE: Making existing internalized strings external doesn't really do anything
    // other than report back when the string is collected.  The externalized resource
    // is not actually used by JSC.
    if (resource->length() > v8::String::kMaxLength) {
        return false;
    }
    auto impl = ToImpl<V82JSC::String>(this);
    auto iso = ToIsolateImpl(impl);
    impl->m_map = ToV8Map(iso->m_external_one_byte_string_map);

    V82JSC::WeakExternalString::Init(iso, impl->m_value, resource, iso->m_weak_external_one_byte_string_map);
    
    * reinterpret_cast<ExternalOneByteStringResource**>(reinterpret_cast<intptr_t>(impl) +
                                                 internal::Internals::kStringResourceOffset) = resource;

    return true;
}

/**
 * Returns true if this string can be made external.
 */
bool v8::String::CanMakeExternal()
{
    return true;
}

void V82JSC::WeakExternalString::Init(IsolateImpl* iso,
                                      JSValueRef value,
                                      v8::String::ExternalStringResourceBase *resource,
                                      BaseMap *map)
{
    HandleScope scope(ToIsolate(iso));
    auto ext = static_cast<WeakExternalString*>(HeapAllocator::Alloc(iso, map));
    ext->m_value = value;
    ext->m_weakRef = JSWeakCreate(iso->m_group, (JSObjectRef)value);
    ext->m_resource = resource;
    assert(JSWeakGetObject(ext->m_weakRef) == (JSObjectRef)value);
    Local<v8::WeakExternalString> wes = CreateLocal<v8::WeakExternalString>(&iso->ii, ext);
    iso->m_external_strings[ext->m_value].Reset(ToIsolate(iso), wes);
}

