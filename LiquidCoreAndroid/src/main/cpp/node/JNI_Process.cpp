//
// JNI_Process.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.

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
