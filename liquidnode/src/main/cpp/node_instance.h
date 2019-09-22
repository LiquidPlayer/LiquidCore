/*
 * Copyright (c) 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JavaScriptCore/JavaScript.h"

struct NodeInstance {
    static void NotifyStart(JSContextRef ctxRef, JSContextGroupRef groupRef);
    static v8::Local<v8::Context> NewContext(v8::Isolate *isolate, JSContextGroupRef groupRef,
                                      JSGlobalContextRef *ctxRef);
    static JSContextGroupRef GroupFromIsolate(v8::Isolate *isolate, uv_loop_t* event_loop);
};

namespace node {
    void InitializeNode();
    v8::Platform * GetPlatform();
}