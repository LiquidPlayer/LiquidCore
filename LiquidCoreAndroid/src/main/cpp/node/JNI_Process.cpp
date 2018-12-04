/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JNI/JNI.h"
#include "NodeInstance.h"

#undef NATIVE
#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_node_##package##_##f
#undef PARAMS
#define PARAMS JNIEnv* env, jobject thiz

NATIVE(Process,jlong,start) (PARAMS)
{
    NodeInstance *instance = new NodeInstance(env, thiz);
    return reinterpret_cast<jlong>(instance);
}

NATIVE(Process,void,runInThread) (PARAMS, jlong ref)
{
    NodeInstance *instance = reinterpret_cast<NodeInstance*>(ref);
    instance->spawnedThread();
}

NATIVE(Process,void,dispose) (PARAMS, jlong ref)
{
    delete reinterpret_cast<NodeInstance*>(ref);
}

NATIVE(Process,void,setFileSystem) (PARAMS, jlong contextRef, jlong fsObjectRef)
{
    auto ctx = SharedWrap<JSContext>::Shared(contextRef);
    if (!ISPOINTER(fsObjectRef)) {
        __android_log_assert("!ISPOINTER", "setFileSystem", "SharedWrap<JSValue> is not a pointer");
    }
    auto fs = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(),fsObjectRef);
    V8_ISOLATE_CTX(ctx,isolate,context)

        Local<Object> globalObj = context->Global();
        Local<Object> fsObj = fs->Value()->ToObject(context).ToLocalChecked();

        Local<Private> privateKey = v8::Private::ForApi(isolate,
                                                        String::NewFromUtf8(isolate, "__fs"));
        globalObj->SetPrivate(context, privateKey, fsObj);

    V8_UNLOCK();
}
