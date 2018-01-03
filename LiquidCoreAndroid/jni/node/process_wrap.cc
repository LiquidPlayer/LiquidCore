//
// process_wrap.cc
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Edited by Eric Lange
//
// Based on process_wrap.cc in node/src/process_wrap.cc
/*
 Copyright (c) 2016-17 Eric Lange. All rights reserved.

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
