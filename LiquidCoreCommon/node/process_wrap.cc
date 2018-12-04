/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "env.h"
#include "env-inl.h"
#include "handle_wrap.h"
#include "node_wrap.h"
#include "util.h"
#include "util-inl.h"

#include <string.h>
#include <stdlib.h>

namespace nodedroid {

using v8::Local;
using v8::Object;
using v8::Value;
using v8::Context;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;

using node::HandleWrap;
using node::Environment;

class ProcessWrap : public HandleWrap {
 public:
  static void Initialize(Local<Object> target,
                         Local<Value> unused,
                         Local<Context> context) {
    Environment* env = Environment::GetCurrent(context);
    Local<FunctionTemplate> constructor = env->NewFunctionTemplate(New);
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "Process"));

    // Disable all process manipulation capabilities for nodedroid
    // We don't want mini-apps to have this much control
    /*
    env->SetProtoMethod(constructor, "close", HandleWrap::Close);

    env->SetProtoMethod(constructor, "spawn", Spawn);
    env->SetProtoMethod(constructor, "kill", Kill);

    env->SetProtoMethod(constructor, "ref", HandleWrap::Ref);
    env->SetProtoMethod(constructor, "unref", HandleWrap::Unref);
    env->SetProtoMethod(constructor, "hasRef", HandleWrap::HasRef);
    */

    target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "Process"),
                constructor->GetFunction());
  }

  size_t self_size() const override { return sizeof(*this); }

 private:
  static void New(const FunctionCallbackInfo<Value>& args) {
    // This constructor should not be exposed to public javascript.
    // Therefore we assert that we are not trying to call this as a
    // normal function.
    CHECK(args.IsConstructCall());
    Environment* env = Environment::GetCurrent(args);
    new ProcessWrap(env, args.This());
  }

  ProcessWrap(Environment* env, Local<Object> object)
      : HandleWrap(env,
                   object,
                   reinterpret_cast<uv_handle_t*>(&process_),
                   AsyncWrap::PROVIDER_PROCESSWRAP) {
  }

  uv_process_t process_;
};

}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(process_wrap, nodedroid::ProcessWrap::Initialize)
