//
// SharedWrap.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.

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
#ifndef LIQUIDCORE_SHAREDWRAP_H
#define LIQUIDCORE_SHAREDWRAP_H

#include "Common/Common.h"
#include <shared_mutex>

template<typename T>
class SharedWrap {
public:
    inline static jlong New(boost::shared_ptr<T> shared)
    {
        if (!shared) return 0L;
        shared->retainJavaReference();
        return reinterpret_cast<jlong>(&* shared);
    }
    inline static boost::shared_ptr<T> Shared(jlong thiz)
    {
        static_assert(!std::is_same<T, JSValue>::value, "SharedWrap<JSValue> requires a JSContext");
        if (!thiz) return boost::shared_ptr<T>();
        return reinterpret_cast<T*>(thiz)->javaReference();
    }
    inline static boost::shared_ptr<T> Shared(boost::shared_ptr<JSContext> context, jlong thiz)
    {
        return Shared(thiz);
    }
    inline static void Dispose(jlong reference)
    {
        auto shared = reinterpret_cast<T*>(reference)->javaReference();
        if (shared) {
            if (shared->Group()->Loop() != nullptr && !shared->IsDefunct()) {
                shared->Group()->MarkZombie(shared);
            }
            shared->releaseJavaReference();
            shared.reset();
        }
    }
};


template<>
inline jlong SharedWrap<JSValue>::New(boost::shared_ptr<JSValue> shared)
{
    if (!shared) return 0L;
    if (ISPOINTER(shared->jniReference())) {
        shared->retainJavaReference();
    }

    return shared->jniReference();
}
template<>
inline boost::shared_ptr<JSValue> SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext> context,
                                                              jlong thiz)
{
    return JSValue::New(context, thiz);
}
template<>
inline void SharedWrap<JSValue>::Dispose(jlong reference)
{
    if (ISPOINTER(reference)) {
        auto shared = TOJSVALUE(reference)->javaReference();
        if (shared) {
            if (shared->Group()->Loop() != nullptr && !shared->IsDefunct()) {
                shared->Group()->MarkZombie(shared);
            }
            shared->releaseJavaReference();
            shared.reset();
        }
    }
}

#endif //LIQUIDCORE_SHAREDWRAP_H
