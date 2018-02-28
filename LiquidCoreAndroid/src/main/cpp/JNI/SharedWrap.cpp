//
// SharedWrap.cpp
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
#include "JNI/JNI.h"
#include "JNI/SharedWrap.h"
#include "Common/LoopPreserver.h"

static jfieldID s_fid;

template<typename T>
SharedWrap<T>::SharedWrap(boost::shared_ptr<T> g) : m_shared(g)
{
    m_isAsync = g->Group()->Loop() != nullptr;
}

template<typename T>
SharedWrap<T>::~SharedWrap()
{
    boost::shared_ptr<T> shared = m_shared;
    if (shared) {
        shared.reset();
    }
}

template<typename T>
jlong SharedWrap<T>::New(JNIEnv *env, boost::shared_ptr<T> shared)
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

template<typename T>
boost::shared_ptr<T> SharedWrap<T>::Shared(JNIEnv *env, jlong thiz)
{
    if (!thiz) return boost::shared_ptr<T>();
    boost::shared_ptr<T> w = GetWrap(env, thiz)->m_shared;
    return w;
}

template<typename T>
void SharedWrap<T>::Dispose(jlong reference)
{
    const auto valueWrap = reinterpret_cast<SharedWrap<T>*>(reference);

    boost::shared_ptr<T> shared = valueWrap->m_shared;
    shared->setJavaReference(0);
    if (valueWrap->m_isAsync && !shared->IsDefunct()) {
        shared->Group()->MarkZombie(valueWrap->m_shared);
    }
    delete valueWrap;
}

template<typename T>
SharedWrap<T>* SharedWrap<T>::GetWrap(JNIEnv *env, jlong thiz)
{
    return reinterpret_cast<SharedWrap<T> *>(thiz);
}


template class SharedWrap<ContextGroup>;
template class SharedWrap<JSContext>;
template class SharedWrap<JSValue>;
template class SharedWrap<LoopPreserver>;
