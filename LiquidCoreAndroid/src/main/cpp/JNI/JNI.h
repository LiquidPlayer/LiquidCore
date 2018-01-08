//
// JSJNI.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
#ifndef NODEDROID_JSJNI_H
#define NODEDROID_JSJNI_H

#include "Common/Common.h"
#include "JNI/SharedWrap.h"

#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_javascript_##package##_##f
#define PARAMS JNIEnv* env, jobject thiz

class JNIReturnObject {
public:
    JNIReturnObject(JNIEnv *env) : m_env(env)
    {
        m_class = findClass(env, "org/liquidplayer/javascript/JNIReturnObject");
        jmethodID cid = env->GetMethodID(m_class,"<init>","()V");
        m_out = m_env->NewObject(m_class, cid);
    }

    JNIReturnObject(JNIEnv *env, jobject obj) : m_env(env), m_out(obj)
    {
        m_class = findClass(env, "org/liquidplayer/javascript/JNIReturnObject");
    }

    virtual jobject ToJava()
    {
        return m_out;
    }

    virtual void SetReference(jobject ref)
    {
        jfieldID fid = m_env->GetFieldID(m_class, "reference",
            "Lorg/liquidplayer/javascript/JNIObject;");
        m_env->SetObjectField(m_out, fid, ref);
    }

    virtual jobject GetReference()
    {
        jfieldID fid = m_env->GetFieldID(m_class, "reference",
            "Lorg/liquidplayer/javascript/JNIObject;");
        return m_env->GetObjectField(m_out, fid);
    }

    virtual void SetException(jobject ref)
    {
        jfieldID fid = m_env->GetFieldID(m_class, "exception",
            "Lorg/liquidplayer/javascript/JNIObject;");
        m_env->SetObjectField(m_out, fid, ref);
    }

    virtual jobject GetException()
    {
        jfieldID fid = m_env->GetFieldID(m_class, "exception",
            "Lorg/liquidplayer/javascript/JNIObject;");
        return m_env->GetObjectField(m_out, fid);
    }

    virtual void SetBool(bool b)
    {
        jfieldID fid = m_env->GetFieldID(m_class ,"bool", "Z");
        m_env->SetBooleanField(m_out, fid, b);
    }

    virtual void SetNumber(double d)
    {
        jfieldID fid = m_env->GetFieldID(m_class ,"number", "D");
        m_env->SetDoubleField(m_out, fid, d);
    }

    virtual void SetString(jstring string)
    {
        jfieldID fid = m_env->GetFieldID(m_class , "string", "Ljava/lang/String;");
        m_env->SetObjectField(m_out, fid, string);
    }

private:
    jclass m_class;
    JNIEnv *m_env;
    jobject m_out;
};

#endif //NODEDROID_JSJNI_H
