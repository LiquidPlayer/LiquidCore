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
#include <mutex>
#include <shared_mutex>

template<typename T>
class SharedWrap {
public:
    inline SharedWrap(boost::shared_ptr<T> g) : m_shared(g)
    {

    }
    inline ~SharedWrap()
    {
        boost::shared_ptr<T> shared = m_shared;
        if (shared) {
            shared.reset();
        }
    }

    inline static jlong New(boost::shared_ptr<T> shared)
    {
        if (!shared) return 0L;

        jlong javao = shared->getJavaReference();
        if (!javao) {
            SharedWrap<T> *wrap = new SharedWrap<T>(shared);
            javao = reinterpret_cast<jlong>(wrap);
            shared->setJavaReference(javao);
        }
        return javao;
    }
    inline static boost::shared_ptr<T> Shared(jlong thiz)
    {
        if (!thiz) return boost::shared_ptr<T>();
        boost::shared_ptr<T> w = GetWrap(thiz)->m_shared;
        return w;
    }
    inline static void Dispose(jlong reference)
    {
        const auto valueWrap = reinterpret_cast<SharedWrap<T>*>(reference);

        boost::shared_ptr<T> shared = valueWrap->m_shared;
        shared->setJavaReference(0);
        if (shared->Group()->Loop() != nullptr && !shared->IsDefunct()) {
            shared->Group()->MarkZombie(shared);
        }
        delete valueWrap;
    }

private:
    inline static SharedWrap<T>* GetWrap(jlong thiz)
    {
        return reinterpret_cast<SharedWrap<T> *>(thiz);
    }

    boost::atomic_shared_ptr<T> m_shared;
};

#endif //LIQUIDCORE_SHAREDWRAP_H
