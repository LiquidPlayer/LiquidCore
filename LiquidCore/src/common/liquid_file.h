/*
 * Copyright (c) 2016 - 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUID_FILE_H_
#define LIQUID_FILE_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#include "node.h"
#include "v8.h"
#include "uv.h"

namespace node {
namespace fs {

#define _FS_ACCESS_RD (1)
#define _FS_ACCESS_WR (2)
#define _FS_ACCESS_NONE (0)

v8::Local<v8::Value> alias_(node::Environment *env, v8::Local<v8::Value> path);
v8::Local<v8::Value> fs_(node::Environment *env, v8::Local<v8::Value> path, int req_access);
void Chdir(const v8::FunctionCallbackInfo<v8::Value>& args);
void Cwd(const v8::FunctionCallbackInfo<v8::Value>& args);

}}

#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#endif  // LIQUID_FILE_H_
