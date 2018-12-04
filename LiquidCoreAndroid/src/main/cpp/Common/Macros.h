/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
*/
#ifndef LIQUIDCORE_MACROS_H
#define LIQUIDCORE_MACROS_H

#include "Common/JSContext.h"

#define V8_ISOLATE(group,iso) \
        boost::shared_ptr<ContextGroup> group_ = (group); \
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
