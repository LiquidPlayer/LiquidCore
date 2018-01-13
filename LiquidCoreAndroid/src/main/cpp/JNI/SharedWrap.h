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

template<typename T>
class SharedWrap {
public:
    SharedWrap(boost::shared_ptr<T> g);
    virtual ~SharedWrap();

    static jobject New(JNIEnv *env, boost::shared_ptr<T> shared);
    static boost::shared_ptr<T> Shared(JNIEnv *env, jobject thiz);
    static void Dispose(long reference);

private:
    static SharedWrap<T>* GetWrap(JNIEnv *env, jobject thiz);
    static const char * ClassName();
    static const char * ClassName(boost::shared_ptr<T> shared);

    static std::map<T *, jobject> s_jobject_map;
    static std::mutex s_mutex;

    boost::atomic_shared_ptr<T> m_shared;
    bool m_isAsync;
};

#endif //LIQUIDCORE_SHAREDWRAP_H
