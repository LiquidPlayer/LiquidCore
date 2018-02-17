//
// JNIReturnObject.h
//
// AndroidJSCore project
// https://github.com/ericwlange/AndroidJSCore/
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
#ifndef LIQUIDCORE_JNIRETURNOBJECT_H
#define LIQUIDCORE_JNIRETURNOBJECT_H

#include "JNI/JNI.h"

class JNIReturnObject {
public:
    JNIReturnObject(JNIEnv *env) : m_env(env)
    {
        init();
        m_out = m_env->NewObject(m_clazz, m_cid);
    }

    JNIReturnObject(JNIEnv *env, jobject obj) : m_env(env), m_out(obj)
    {
        init();
    }

    inline jobject ToJava()
    {
        return m_out;
    }

    inline void SetReference(jobject ref)
    {
        m_env->SetObjectField(m_out, m_referenceFid, ref);
    }

    inline jobject GetReference()
    {
        return m_env->GetObjectField(m_out, m_referenceFid);
    }

    inline void SetException(jobject ref) {
        m_env->SetObjectField(m_out, m_exceptionFid, ref);
    }

    inline jobject GetException()
    {
        return m_env->GetObjectField(m_out, m_exceptionFid);
    }

    inline void SetBool(bool b)
    {
        m_env->SetBooleanField(m_out, m_boolFid, (jboolean) b);
    }

    inline void SetNumber(double d)
    {
        m_env->SetDoubleField(m_out, m_numberFid, d);
    }

    inline void SetString(jstring string)
    {
        m_env->SetObjectField(m_out, m_stringFid, string);
    }

private:
    inline void init()
    {
        if (!m_clazz) {
            m_clazz = (jclass) m_env->NewGlobalRef(
                    findClass(m_env, "org/liquidplayer/javascript/JNIReturnObject"));
            m_cid = m_env->GetMethodID(m_clazz, "<init>", "()V");
            m_referenceFid = m_env->GetFieldID(m_clazz, "reference",
                                               "Lorg/liquidplayer/javascript/JNIObject;");
            m_exceptionFid = m_env->GetFieldID(m_clazz, "exception",
                                               "Lorg/liquidplayer/javascript/JNIObject;");
            m_boolFid = m_env->GetFieldID(m_clazz, "bool", "Z");
            m_numberFid = m_env->GetFieldID(m_clazz, "number", "D");
            m_stringFid = m_env->GetFieldID(m_clazz, "string", "Ljava/lang/String;");
        }
    }

    static jfieldID m_referenceFid;
    static jfieldID m_exceptionFid;
    static jfieldID m_boolFid;
    static jfieldID m_numberFid;
    static jfieldID m_stringFid;
    static jmethodID m_cid;

    static jclass m_clazz;

    JNIEnv *m_env;
    jobject m_out;
};

#endif //LIQUIDCORE_JNIRETURNOBJECT_H
