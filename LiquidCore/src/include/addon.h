/*
 * Copyright (c) 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef _LIQUIDCORE_ADDON_H
#define _LIQUIDCORE_ADDON_H

#include "v8.h"

namespace LiquidCore
{

inline v8::MaybeLocal<v8::Value> resolve(v8::Local<v8::Value> filename)
{
  v8::Local<v8::String> s_resolve = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(),
          "process.binding('fs').resolve", v8::String::NewStringType::kNormalString);

  v8::Local<v8::Context> context = v8::Isolate::GetCurrent()->GetCurrentContext();
  v8::Local<v8::Script> script = v8::Script::Compile(context, s_resolve).ToLocalChecked();
  v8::Local<v8::Function> resolve = script->Run(context).ToLocalChecked().As<v8::Function>();

  v8::MaybeLocal<v8::Value> location = resolve->Call(context,
   v8::Undefined(v8::Isolate::GetCurrent()), 1, &filename);
  return location;
}

}

#endif//_LIQUIDCORE_ADDON_H
