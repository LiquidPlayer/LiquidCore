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
        jclass clazz = findClass(m_env, "org/liquidplayer/javascript/JNIReturnObject");
        jmethodID cid = env->GetMethodID(clazz,"<init>","()V");
        m_out = m_env->NewObject(clazz, cid);
        m_env->DeleteLocalRef(clazz);
    }

    JNIReturnObject(JNIEnv *env, jobject obj) : m_env(env), m_out(obj)
    {
    }

    jobject ToJava()
    {
        return m_out;
    }

    void SetReference(jobject ref)
    {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz, "reference", "Lorg/liquidplayer/javascript/JNIObject;");
        });
        m_env->SetObjectField(m_out, fid, ref);
    }

    jobject GetReference()
    {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz, "reference", "Lorg/liquidplayer/javascript/JNIObject;");
        });
        return m_env->GetObjectField(m_out, fid);
    }

    void SetException(jobject ref) {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz, "exception", "Lorg/liquidplayer/javascript/JNIObject;");
        });
        m_env->SetObjectField(m_out, fid, ref);
    }

    jobject GetException()
    {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz, "exception", "Lorg/liquidplayer/javascript/JNIObject;");
        });
        return m_env->GetObjectField(m_out, fid);
    }

    void SetBool(bool b)
    {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz, "bool", "Z");
        });
        m_env->SetBooleanField(m_out, fid, (jboolean) b);
    }

    void SetNumber(double d)
    {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz ,"number", "D");
        });
        m_env->SetDoubleField(m_out, fid, d);
    }

    void SetString(jstring string)
    {
        jfieldID fid = LambdaField([&](jclass clazz) -> jfieldID {
            return m_env->GetFieldID(clazz, "string", "Ljava/lang/String;");
        });
        m_env->SetObjectField(m_out, fid, string);
    }

private:
    template <typename F>
    jfieldID LambdaField(F&& f) {
        jclass clazz = findClass(m_env, "org/liquidplayer/javascript/JNIReturnObject");
        jfieldID fid = f(clazz);
        m_env->DeleteLocalRef(clazz);
        return fid;
    }

    JNIEnv *m_env;
    jobject m_out;
};

#endif //LIQUIDCORE_JNIRETURNOBJECT_H
