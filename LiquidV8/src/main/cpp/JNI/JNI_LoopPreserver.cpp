/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */

#include "JNI/JNI.h"
#include "Common/LoopPreserver.h"

NATIVE(JNILoopPreserver,jlong,create) (STATIC, jlong grpRef)
{
    auto group = SharedWrap<ContextGroup>::Shared(grpRef);

    if (group && group->Loop()) {
        return SharedWrap<LoopPreserver>::New(LoopPreserver::New(group));
    }

    return 0;
}

NATIVE(JNILoopPreserver,void,release) (STATIC, jlong loopRef)
{
    SharedWrap<LoopPreserver>::Shared(loopRef)->Dispose();
}

NATIVE(JNILoopPreserver,void,Finalize) (STATIC, jlong loopRef)
{
    SharedWrap<LoopPreserver>::Dispose(loopRef);
}

