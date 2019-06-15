/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_OPAQUEJSSTRING_H
#define LIQUIDCORE_OPAQUEJSSTRING_H

#include "JSC/JSCRetainer.h"
#include "JavaScriptCore/JavaScript.h"

struct OpaqueJSString : public JSCRetainer {
    public:
        static JSStringRef New(Local<String> string);
        static JSStringRef New(const JSChar * chars, size_t numChars);
        static JSStringRef New(const char * chars);
        OpaqueJSString(Local<String> string);
        OpaqueJSString(const JSChar * chars, size_t numChars);
        OpaqueJSString(const char * chars);
        virtual ~OpaqueJSString();
        Local<String> Value(Isolate *);
        const JSChar * Chars();
        size_t Size();
        size_t Utf8Bytes();
        void Utf8String(std::string&);
        bool Equals(OpaqueJSString& other);

    private:
        std::vector<unsigned short> backstore;
        bool m_isNull;
};

#endif //LIQUIDCORE_OPAQUEJSSTRING_H
