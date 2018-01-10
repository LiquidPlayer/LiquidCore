//
// Created by Eric Lange on 1/10/18.
//

#ifndef LIQUIDCORE_JNIRETURNOBJECT_H
#define LIQUIDCORE_JNIRETURNOBJECT_H

#include "JNI/JNI.h"

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

    jobject ToJava()
    {
        return m_out;
    }

    void SetReference(jobject ref)
    {
        jfieldID fid = m_env->GetFieldID(m_class, "reference",
                                         "Lorg/liquidplayer/javascript/JNIObject;");
        m_env->SetObjectField(m_out, fid, ref);
    }

    jobject GetReference()
    {
        jfieldID fid = m_env->GetFieldID(m_class, "reference",
                                         "Lorg/liquidplayer/javascript/JNIObject;");
        return m_env->GetObjectField(m_out, fid);
    }

    void SetException(jobject ref)
    {
        jfieldID fid = m_env->GetFieldID(m_class, "exception",
                                         "Lorg/liquidplayer/javascript/JNIObject;");
        m_env->SetObjectField(m_out, fid, ref);
    }

    jobject GetException()
    {
        jfieldID fid = m_env->GetFieldID(m_class, "exception",
                                         "Lorg/liquidplayer/javascript/JNIObject;");
        return m_env->GetObjectField(m_out, fid);
    }

    void SetBool(bool b)
    {
        jfieldID fid = m_env->GetFieldID(m_class ,"bool", "Z");
        m_env->SetBooleanField(m_out, fid, b);
    }

    void SetNumber(double d)
    {
        jfieldID fid = m_env->GetFieldID(m_class ,"number", "D");
        m_env->SetDoubleField(m_out, fid, d);
    }

    void SetString(jstring string)
    {
        jfieldID fid = m_env->GetFieldID(m_class , "string", "Ljava/lang/String;");
        m_env->SetObjectField(m_out, fid, string);
    }

private:
    jclass m_class;
    JNIEnv *m_env;
    jobject m_out;
};

#endif //LIQUIDCORE_JNIRETURNOBJECT_H
