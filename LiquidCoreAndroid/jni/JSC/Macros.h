//
// Macros.h
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

#ifndef LIQUIDCORE_JSC_MACROS_H
#define LIQUIDCORE_JSC_MACROS_H

#include <list>

#define ASSERTJSC(x) if(!(x)) \
    __android_log_assert("conditional", "ASSERT FAILED", "%s(%d) : %s", __FILE__, __LINE__, #x);
#ifndef ASSERT
#define ASSERT ASSERTJSC
#endif

/* Reserve position 0 -- node uses it for some objects */
#define INSTANCE_OBJECT_RESERVED (0)
#define INSTANCE_OBJECT_CLASS    (1)
#define INSTANCE_OBJECT_JSOBJECT (2)
#define INSTANCE_OBJECT_FIELDS   (3)

#define V8_ISOLATE_OBJ(ctx,object,isolate,context,o) \
    V8_ISOLATE_CTX(ctx,isolate,context); \
    Local<Object> o = \
        (object)->L()->ToObject(context).ToLocalChecked();

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> value = (valueRef)->L();

#define V8_ISOLATE_CALLBACK(info,isolate,context,definition) \
    Isolate::Scope isolate_scope_(info.GetIsolate()); \
    HandleScope handle_scope_(info.GetIsolate()); \
    const JSClassDefinition *definition = ObjectData::Get(info.Data())->Definition();\
    if (nullptr == ObjectData::Get(info.Data())->Context()) return; \
    JSContextRef ctxRef_ = ObjectData::Get(info.Data())->Context(); \
    V8_ISOLATE_CTX(ctxRef_->Context(),isolate,context)

#define TO_REAL_GLOBAL(o) \
    o = o->StrictEquals(context->Global()) && \
        !o->GetPrototype()->ToObject(context).IsEmpty() && \
        o->GetPrototype()->ToObject(context).ToLocalChecked()->InternalFieldCount()>INSTANCE_OBJECT_JSOBJECT ? \
        o->GetPrototype()->ToObject(context).ToLocalChecked() : \
        o;

#define CTX(ctx)     ((ctx)->Context())

#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       OpaqueJSValue

#endif //LIQUIDCORE_JSC_MACROS_H
