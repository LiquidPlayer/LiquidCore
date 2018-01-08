//
// Macros.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 - 2018 Eric Lange. All rights reserved.

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
#ifndef LIQUIDCORE_MACROS_H
#define LIQUIDCORE_MACROS_H

#include "Common/JSContext.h"

#define V8_ISOLATE(group,iso) \
        std::shared_ptr<ContextGroup> group_ = (group); \
        auto runnable_ = [&]() \
        { \
            Isolate *iso = group_->isolate(); \
            v8::Locker lock_(group_->isolate()); \
            Isolate::Scope isolate_scope_(iso); \
            HandleScope handle_scope_(iso);

#define V8_ISOLATE_CTX(ctx,iso,Ctx) \
        V8_ISOLATE(ctx->Group(),iso) \
            Local<v8::Context> Ctx = ctx->Value(); \
            v8::Context::Scope context_scope_(Ctx);

#define V8_UNLOCK() \
        }; group_->sync(runnable_);

#endif //LIQUIDCORE_MACROS_H
