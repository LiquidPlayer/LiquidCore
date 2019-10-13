/*
 * Copyright (c) 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "node.h"
#include <jni.h>

using namespace v8;

void LowLevelFunction(const FunctionCallbackInfo<Value> &info )
{
    HandleScope scope(info.GetIsolate());
    info.GetReturnValue().Set(43);
}

void Init(Local<Object> target)
{
    auto const isolate = target->GetIsolate();
    HandleScope scope(isolate);
    auto fnName = String::NewFromUtf8(
            isolate,
            "lowLevelFunction",
            String::kNormalString
            );
    auto fnTempl = FunctionTemplate::New(isolate, LowLevelFunction);
    target->Set(
            target->CreationContext(),
            fnName,
            fnTempl->GetFunction()
            );
}

NODE_MODULE_CONTEXT_AWARE(nativeModuleTest,Init)

extern "C" void JNICALL Java_org_liquidplayer_addon_NativeModuleTest_register(JNIEnv* env, jobject thiz)
{
    _register_nativeModuleTest();
}
