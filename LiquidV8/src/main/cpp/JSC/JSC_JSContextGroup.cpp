/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JSC/JSC.h"

JS_EXPORT JSContextGroupRef JSContextGroupCreate()
{
    return &* OpaqueJSContextGroup::New();
}

JS_EXPORT JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group)
{
    const_cast<OpaqueJSContextGroup*>(group)->Retain();
    return group;
}

JS_EXPORT void JSContextGroupRelease(JSContextGroupRef group)
{
    const_cast<OpaqueJSContextGroup*>(group)->Release();
}
