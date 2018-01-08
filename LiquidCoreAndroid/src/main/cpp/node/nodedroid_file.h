//
// nodedroid_file.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Edited by Eric Lange
//
// Based on node_file.h in node/src/node_file.h
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
#ifndef SRC_NODE_FILE_H_
#define SRC_NODE_FILE_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#include "node.h"
#include "v8.h"

namespace nodedroid {

#define _FS_ACCESS_RD (1)
#define _FS_ACCESS_WR (2)
#define _FS_ACCESS_NONE (0)

void InitFs(v8::Local<v8::Object> target);
v8::Local<v8::Value> alias_(node::Environment *env, v8::Local<v8::Value> path);
v8::Local<v8::Value> fs_(node::Environment *env, v8::Local<v8::Value> path, int req_access);
v8::Local<v8::Value> chdir_(node::Environment *env, v8::Local<v8::Value> path);
v8::Local<v8::Value> cwd_(node::Environment *env);
void FillStatsArray(double* fields, const uv_stat_t* s);

extern "C" node::node_module fs_module;

}  // namespace nodedroid

#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#endif  // SRC_NODE_FILE_H_
