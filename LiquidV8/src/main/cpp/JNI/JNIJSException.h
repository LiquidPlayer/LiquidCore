/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_JNIJSEXCEPTION_H
#define LIQUIDCORE_JNIJSEXCEPTION_H

#include "JNI/JNI.h"

class JNIJSException {
public:
    inline JNIJSException(JNIEnv *env, jlong exception) : m_env(env), m_exception(exception)
    {
        if (!m_clazz) {
            m_clazz = (jclass) m_env->NewGlobalRef(
                    findClass(m_env, "org/liquidplayer/javascript/JNIJSException"));
            m_cid = m_env->GetMethodID(m_clazz, "<init>", "(J)V");
        }
        if (m_exception) {
            m_out = (jthrowable) m_env->NewObject(m_clazz, m_cid, exception);
        } else {
            m_out = nullptr;
        }
    }

    void Throw()
    {
        if (m_exception) {
            m_env->Throw(m_out);
        }
    }

private:
    static jmethodID m_cid;
    static jclass m_clazz;

    JNIEnv *m_env;
    jthrowable m_out;
    jlong m_exception;
};


#endif //LIQUIDCORE_JNIJSEXCEPTION_H
