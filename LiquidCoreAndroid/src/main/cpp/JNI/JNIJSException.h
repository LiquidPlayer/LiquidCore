//
// JNIJSException.h
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
