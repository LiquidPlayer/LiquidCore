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

MaybeLocal<String> String::NewFromUtf8(Isolate* isolate, const char* data,
                                       v8::NewStringType type, int length)
{
    StringImpl * string = (StringImpl *) malloc(sizeof(StringImpl));
    memset(string, 0, sizeof(StringImpl));
    
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
