//
// ObjectData.h
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

#ifndef LIQUIDCORE_OBJECTDATA_H
#define LIQUIDCORE_OBJECTDATA_H

#include "JavaScriptCore/JavaScript.h"
#include "JSC/JSCRetainer.h"

class ObjectData {
    public:
        static Local<Value> New(const JSClassDefinition *def = nullptr, JSContextRef ctx = nullptr,
                               JSClassRef cls = nullptr);
        static ObjectData* Get(Local<Value> value);
        void SetContext(JSContextRef ctx);
        void SetName(Local<Value> name);
        void SetFunc(Local<Object> func);
        inline const JSClassDefinition *Definition() { return m_definition; }
        inline JSContextRef       Context() { return m_context; }
        inline JSClassRef         Class() { return m_class; }
        inline const char *       Name() { return m_name; }
        inline Local<Object>      Func() { return Local<Object>::New(Isolate::GetCurrent(), m_func); }

    protected:
        ~ObjectData();

    private:
        ObjectData(const JSClassDefinition *def, JSContextRef ctx, JSClassRef cls);

        const JSClassDefinition*m_definition;
        JSContextRef            m_context;
        JSClassRef              m_class;
        UniquePersistent<Value> m_weak;
        char *                  m_name;
        Persistent<Object, CopyablePersistentTraits<Object>> m_func;
};

#endif //LIQUIDCORE_OBJECTDATA_H
