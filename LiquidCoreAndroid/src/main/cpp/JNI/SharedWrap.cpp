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
#include "JNI/SharedWrap.h"

template<typename T>
SharedWrap<T>::SharedWrap(std::shared_ptr<T> g) : m_shared(g)
{
    m_isAsync = g->Group()->Loop() != nullptr;
}

template<typename T>
SharedWrap<T>::~SharedWrap()
{
    if (m_shared) {
        s_mutex.lock();
        s_jobject_map.erase(&*m_shared);
        s_mutex.unlock();
    }

    //m_shared.reset();
}

template<typename T>
jobject SharedWrap<T>::New(JNIEnv *env, std::shared_ptr<T> shared)
{
    if (!shared) return nullptr;

    jobject javao = nullptr;

    s_mutex.lock();
    if (s_jobject_map.count(&*shared) == 1) {
        javao = s_jobject_map[&*shared];
        if (env->IsSameObject(javao, nullptr)) {
            // If Finalize() is being called correctly, this shouldn't happen
            javao = nullptr;
            s_jobject_map.erase(&*shared);
        }
    }
    s_mutex.unlock();

    if (!javao) {
        SharedWrap<T> *wrap = new SharedWrap<T>(shared);
        jclass classType = findClass(env, ClassName(shared));
        jmethodID cid = env->GetMethodID(classType,"<init>","(J)V");
        javao = env->NewObject(classType, cid, reinterpret_cast<jlong>(wrap));
        s_mutex.lock();
        s_jobject_map[&*shared] = env->NewWeakGlobalRef(javao);
        s_mutex.unlock();
    }
    return javao;
}

template<typename T>
std::shared_ptr<T> SharedWrap<T>::Shared(JNIEnv *env, jobject thiz)
{
    std::shared_ptr<T> s = thiz ? GetWrap(env, thiz)->m_shared : std::shared_ptr<T>();
    return s;
}

template<typename T>
void SharedWrap<T>::Dispose(long reference)
{
    const auto valueWrap = reinterpret_cast<SharedWrap<T>*>(reference);
    if (valueWrap->m_isAsync && !valueWrap->m_shared->IsDefunct()) {
        valueWrap->m_shared->Group()->MarkZombie(valueWrap->m_shared);
    }
    delete valueWrap;
}

template<typename T>
SharedWrap<T>* SharedWrap<T>::GetWrap(JNIEnv *env, jobject thiz)
{
    jclass classType = findClass(env, "org/liquidplayer/javascript/JNIObject");
    jfieldID fid = env->GetFieldID(classType, "reference", "J");
    jlong ref = env->GetLongField(thiz, fid);
    return reinterpret_cast<SharedWrap<T> *>(ref);
}

template<typename T>
const char * SharedWrap<T>::ClassName()
{
    if (std::is_same<T,ContextGroup>::value) {
        return "org/liquidplayer/javascript/JNIJSContextGroup";
    } else if (std::is_same<T,JSContext>::value) {
        return "org/liquidplayer/javascript/JNIJSContext";
    } else if (std::is_same<T,JSValue>::value) {
        return "org/liquidplayer/javascript/JNIJSValue";
    } else if (std::is_same<T,LoopPreserver>::value) {
        return "org/liquidplayer/javascript/JNILoopPreserver";
    }
    return nullptr;
}

template<typename T>
const char * SharedWrap<T>::ClassName(std::shared_ptr<T> shared)
{
    return ClassName();
}

template<>
const char * SharedWrap<JSValue>::ClassName(std::shared_ptr<JSValue> shared)
{
    const char *r = "org/liquidplayer/javascript/JNIJSValue";
    V8_ISOLATE_CTX(shared->Context(), isolate, ctx)
    if (shared && shared->Value()->IsObject()) {
        r =  "org/liquidplayer/javascript/JNIJSObject";
    }
    V8_UNLOCK()
    return r;
}

template<typename T> std::map<T *, jobject> SharedWrap<T>::s_jobject_map;
template<typename T> std::mutex SharedWrap<T>::s_mutex;

template class SharedWrap<ContextGroup>;
template class SharedWrap<JSContext>;
template class SharedWrap<JSValue>;
template class SharedWrap<LoopPreserver>;