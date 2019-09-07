/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_JSC_MACROS_H
#define LIQUIDCORE_JSC_MACROS_H

#include <list>
#include <android/log.h>

#define ASSERTJSC(x) if(!(x)) \
    __android_log_assert("conditional", "ASSERT FAILED", "%s(%d) : %s", __FILE__, __LINE__, #x);
#ifndef ASSERT
#define ASSERT ASSERTJSC
#endif

/* Reserve position 0 -- node uses it for some objects */
#define INSTANCE_OBJECT_JSOBJECT (1)
#define INSTANCE_OBJECT_CLASS    (2)
#define INSTANCE_OBJECT_FIELDS   (3)

#define V8_ISOLATE_OBJ(ctx,object,isolate,context,o) \
    V8_ISOLATE_CTX(ctx,isolate,context); \
    Local<Object> (o) = \
        (object)->L()->ToObject(context).ToLocalChecked();

#define VALUE_ISOLATE(ctxRef,valueRef,isolate,context,value) \
    V8_ISOLATE_CTX(ctxRef,isolate,context); \
    Local<Value> (value) = (valueRef)->L();

#define V8_ISOLATE_CALLBACK(info,isolate,context,definition) \
    Isolate::Scope isolate_scope_((info).GetIsolate()); \
    HandleScope handle_scope_((info).GetIsolate()); \
    const JSClassDefinition *(definition) = ObjectData::Get((info).Data())->Definition();\
    if (nullptr == ObjectData::Get((info).Data())->Context()) return; \
    JSContextRef ctxRef_ = ObjectData::Get((info).Data())->Context(); \
    V8_ISOLATE_CTX(ctxRef_->Context(),isolate,context)

#define TO_REAL_GLOBAL(o) \
    (o) = (o)->StrictEquals(context->Global()) && \
        !(o)->GetPrototype()->ToObject(context).IsEmpty() && \
        (o)->GetPrototype()->ToObject(context).ToLocalChecked()->InternalFieldCount()>INSTANCE_OBJECT_JSOBJECT ? \
        (o)->GetPrototype()->ToObject(context).ToLocalChecked() : \
        (o);

#define CTX(ctx)     ((ctx)->Context())

#define OpaqueJSPropertyNameAccumulator std::list<JSStringRef>
#define OpaqueJSPropertyNameArray       OpaqueJSValue

#endif //LIQUIDCORE_JSC_MACROS_H
