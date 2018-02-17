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
    JNIEnv *env;
    JavaVM *jvm = getJavaVM();

    boost::shared_ptr<T> shared = m_shared;
    if (shared) {
        s_mutex.lock();
        if (s_jobject_map.count(&*shared) == 1) {
            jobject javao = s_jobject_map[&*shared];

            int getEnvStat = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            if (getEnvStat == JNI_EDETACHED) {
                jvm->AttachCurrentThread(&env, NULL);
            }

            env->DeleteWeakGlobalRef(javao);

            if (getEnvStat == JNI_EDETACHED) {
                jvm->DetachCurrentThread();
            }

            s_jobject_map.erase(&*shared);
        }
        s_mutex.unlock();

        shared.reset();
    }
}

template<typename T>
jobject SharedWrap<T>::New(JNIEnv *env, boost::shared_ptr<T> shared)
{
    if (!shared) return nullptr;

    jobject javao = nullptr;

    s_mutex.lock();
    if (s_jobject_map.count(&*shared) == 1) {
        javao = s_jobject_map[&*shared];
        if (env->IsSameObject(javao, nullptr)) {
            // If Finalize() is being called correctly, this shouldn't happen
            env->DeleteWeakGlobalRef(javao);
            javao = nullptr;
            s_mutex.lock();
            s_jobject_map.erase(&*shared);
            s_mutex.unlock();
        }
    }
    s_mutex.unlock();

    if (!javao) {
        SharedWrap<T> *wrap = new SharedWrap<T>(shared);
        jmethodID cid;
        jclass classType = Class(env, shared, cid);
        javao = env->NewObject(classType, cid, reinterpret_cast<jlong>(wrap));
        s_mutex.lock();
        s_jobject_map[&*shared] = env->NewWeakGlobalRef(javao);
        s_mutex.unlock();
    } else {
        javao = env->NewLocalRef(javao);
    }
    return javao;
}

template<typename T>
boost::shared_ptr<T> SharedWrap<T>::Shared(JNIEnv *env, jobject thiz)
{
    if (!thiz) return boost::shared_ptr<T>();
    boost::shared_ptr<T> w = GetWrap(env, thiz)->m_shared;
    return w;
}

template<typename T>
void SharedWrap<T>::Dispose(long reference)
{
    const auto valueWrap = reinterpret_cast<SharedWrap<T>*>(reference);

    boost::shared_ptr<T> shared = valueWrap->m_shared;
    if (valueWrap->m_isAsync && !shared->IsDefunct()) {
        shared->Group()->MarkZombie(valueWrap->m_shared);
    }
    delete valueWrap;
}

template<typename T>
SharedWrap<T>* SharedWrap<T>::GetWrap(JNIEnv *env, jobject thiz)
{
    if (!s_fid) {
        jclass classType = findClass(env, "org/liquidplayer/javascript/JNIObject");
        s_fid = env->GetFieldID(classType, "reference", "J");
        env->DeleteLocalRef(classType);
    }
    jlong ref = env->GetLongField(thiz, s_fid);
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
jclass SharedWrap<T>::Class(JNIEnv *env, boost::shared_ptr<T> shared, jmethodID& mid)
{
    if (!s_class) {
        s_class = (jclass) env->NewGlobalRef(findClass(env, ClassName()));
        s_cid = env->GetMethodID(s_class,"<init>","(J)V");
    }
    mid = s_cid;
    return s_class;
}

template<>
jclass SharedWrap<JSValue>::Class(JNIEnv *env, boost::shared_ptr<JSValue> shared, jmethodID& mid)
{
    if (!s_class) {
        s_class = (jclass) env->NewGlobalRef(
                findClass(env, "org/liquidplayer/javascript/JNIJSValue"));
        s_cid = env->GetMethodID(s_class, "<init>", "(J)V");
    }
    if (!s_class_object) {
        s_class_object = (jclass) env->NewGlobalRef(
                findClass(env, "org/liquidplayer/javascript/JNIJSObject"));
        s_cid_object = env->GetMethodID(s_class_object,"<init>","(J)V");
    }
    jclass classType = s_class;
    mid = s_cid;
    V8_ISOLATE_CTX(shared->Context(), isolate, ctx)
    if (shared && shared->Value()->IsObject()) {
        classType = s_class_object;
        mid = s_cid_object;
    }
    V8_UNLOCK()
    return classType;
}

template<typename T> std::map<T *, jobject> SharedWrap<T>::s_jobject_map;
template<typename T> std::mutex SharedWrap<T>::s_mutex;
template<typename T> jclass SharedWrap<T>::s_class;
template<typename T> jclass SharedWrap<T>::s_class_object;
template<typename T> jmethodID SharedWrap<T>::s_cid;
template<typename T> jmethodID SharedWrap<T>::s_cid_object;

template class SharedWrap<ContextGroup>;
template class SharedWrap<JSContext>;
template class SharedWrap<JSValue>;
template class SharedWrap<LoopPreserver>;
