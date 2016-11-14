//
// Created by Eric on 11/6/16.
//

#include "JSC.h"

class Utf16ExternalStringResource : public String::ExternalStringResource {
    public:
        Utf16ExternalStringResource(const JSChar* chars, size_t numChars) {
            m_chars = (JSChar *) malloc( sizeof(JSChar) * (numChars + 1) );
            memcpy( m_chars, chars, sizeof(JSChar) * (int)numChars );
            m_chars[numChars] = 0;
            m_numChars = numChars;
        }
        virtual ~Utf16ExternalStringResource() {
            free(m_chars);
        }
        const uint16_t* data() const {
            return m_chars;
        }
        size_t length() const {
            return m_numChars;
        }

    private:
        JSChar* m_chars;
        size_t m_numChars;
};

OpaqueJSString::OpaqueJSString(Isolate *isolate, Local<String> string)
{
    m_isolate = isolate;
    m_value = Persistent<String,CopyablePersistentTraits<String>>(isolate, string);
}

OpaqueJSString::~OpaqueJSString()
{
    Isolate::Scope isolate_scope_(m_isolate);
    m_value.Reset();
}

Local<String> OpaqueJSString::Value()
{
    Isolate::Scope isolate_scope_(m_isolate);
    return Local<String>::New(m_isolate, m_value);
}

const JSChar * OpaqueJSString::Chars()
{
    Isolate::Scope isolate_scope_(m_isolate);
    Local<String> string = Value();

    if (!string->IsExternal()) {
        String::Value const str(string);
        Utf16ExternalStringResource *resource =
            new Utf16ExternalStringResource(*str,string->Length());
        string->MakeExternal(resource);
    }

    return string->GetExternalStringResource()->data();
}

JS_EXPORT JSStringRef JSStringCreateWithCharacters(const JSChar* chars, size_t numChars)
{
    Isolate *isolate = Isolate::GetCurrent();
    Isolate::Scope isolate_scope_(isolate);

    Local<String> string =
        String::NewFromTwoByte(isolate, chars, NewStringType::kNormal, numChars).ToLocalChecked();

    return (JSStringRef) new OpaqueJSString(isolate, string);
}

JS_EXPORT JSStringRef JSStringCreateWithUTF8CString(const char* chars)
{
    Isolate *isolate = Isolate::GetCurrent();
    Isolate::Scope isolate_scope_(isolate);

    Local<String> string =
        String::NewFromUtf8(isolate, chars, NewStringType::kNormal).ToLocalChecked();

    return (JSStringRef) new OpaqueJSString(isolate, string);
}

JS_EXPORT JSStringRef JSStringRetain(JSStringRef string)
{
    static_cast<OpaqueJSString *>(string)->retain();
    return string;
}

JS_EXPORT void JSStringRelease(JSStringRef string)
{
    static_cast<OpaqueJSString *>(string)->release();
}

JS_EXPORT size_t JSStringGetLength(JSStringRef string)
{
    OpaqueJSString *value = static_cast<OpaqueJSString *>(string);
    Isolate::Scope isolate_scope_(value->isolate());

    return value->Value()->Length();
}

JS_EXPORT const JSChar* JSStringGetCharactersPtr(JSStringRef string)
{
    OpaqueJSString *value = static_cast<OpaqueJSString *>(string);
    return (JSChar *) value->Chars();
}

JS_EXPORT size_t JSStringGetMaximumUTF8CStringSize(JSStringRef string)
{
    OpaqueJSString *value = static_cast<OpaqueJSString *>(string);
    Isolate::Scope isolate_scope_(value->isolate());

    return value->Value()->Utf8Length();
}

JS_EXPORT size_t JSStringGetUTF8CString(JSStringRef string, char* buffer, size_t bufferSize)
{
    OpaqueJSString *value = static_cast<OpaqueJSString *>(string);
    Isolate::Scope isolate_scope_(value->isolate());

    size_t bytes = (bufferSize > (size_t)value->Value()->Utf8Length()) ?
        (size_t) value->Value()->Utf8Length() : bufferSize;

    String::Utf8Value const str(value->Value());
    memcpy(buffer, *str, bytes);

    return bytes;
}

JS_EXPORT bool JSStringIsEqual(JSStringRef a, JSStringRef b)
{
    OpaqueJSString *a_ = static_cast<OpaqueJSString *>(a);
    OpaqueJSString *b_ = static_cast<OpaqueJSString *>(b);

    Isolate::Scope isolate_scope_(a_->isolate());

    String::Utf8Value const a__(a_->Value());
    String::Utf8Value const b__(b_->Value());

    return strcmp(*a__, *b__) == 0;
}

JS_EXPORT bool JSStringIsEqualToUTF8CString(JSStringRef a, const char* b)
{
    OpaqueJSString *a_ = static_cast<OpaqueJSString *>(a);
    Isolate::Scope isolate_scope_(a_->isolate());

    String::Utf8Value const a__(a_->Value());

    return strcmp(*a__, b) == 0;
}
