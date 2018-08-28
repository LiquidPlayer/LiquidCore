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
#include <shared_mutex>
#include <android/log.h>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class SharedWrap;

template <typename T>
class Queue
{
    public:

    T pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        auto item = queue_.front();
        queue_.pop_front();
        return item;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop_front();
    }

    void push(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push_front(item);
        mlock.unlock();
        cond_.notify_one();
    }

    void push(T&& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push_front(std::move(item));
        mlock.unlock();
        cond_.notify_one();
    }

 private:
    std::deque<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class SharedWrapBase {
public:
    SharedWrapBase()
    {

    }
    virtual ~SharedWrapBase()
    {

    }
    static Queue<SharedWrapBase*> s_zombies;
    static void FreeZombiesThread();
};

template<typename T>
class SharedWrap : SharedWrapBase {
public:
    inline static jlong New(boost::shared_ptr<T> shared)
    {
        if (!shared) return 0L;
        return reinterpret_cast<jlong>(new SharedWrap<T>(shared));
    }
    inline static boost::shared_ptr<T> Shared(jlong thiz)
    {
        static_assert(!std::is_same<T, JSValue>::value, "SharedWrap<JSValue> requires a JSContext");
        if (!thiz) return boost::shared_ptr<T>();
        return reinterpret_cast<SharedWrap<T>*>(thiz)->m_shared;
    }
    inline static boost::shared_ptr<T> Shared(boost::shared_ptr<JSContext> context, jlong thiz)
    {
        return Shared(thiz);
    }
    inline static void Dispose(jlong reference)
    {
        auto wrap = reinterpret_cast<SharedWrap<T>*>(reference);
        s_zombies.push(wrap);
    }
    inline static long CanonicalReference(jlong reference)
    {
        auto wrap = reinterpret_cast<SharedWrap<T>*>(reference);
        boost::shared_ptr<T> shared = wrap->m_shared;
        return reinterpret_cast<long>(&* shared);
    }

protected:
    virtual ~SharedWrap()
    {
        boost::shared_ptr<T> shared = m_shared;
        if (shared) {
            if (shared->Group()->Loop() != nullptr && !shared->IsDefunct()) {
                shared->Group()->MarkZombie(shared);
            }
            shared.reset();
        }
    }

private:
    SharedWrap(boost::shared_ptr<T> shared)
    {
        m_shared = shared;
    };

    boost::atomic_shared_ptr<T> m_shared;
};

/*
 * In order to limit back-and-forth through JNI, for those primitive values that can be
 * represented by a jlong (64-bit integer), we will pass the actual value.  We use the following
 * encoding (2 least significant bits):
 *
 * xxx 00 = 62-bit double
 * xxx 10 = oddball value
 * xxx 01 = 4-byte aligned pointer to non-Object JSValue (63/64-bit double or String)
 * xxx 11 = 4-byte aligned pointer to Object JSValue
 *
 * Oddball values (ending in 10):
 * 0010 (0x2) = Undefined
 * 0110 (0x6) = Null
 * 1010 (0xa) = False
 * 1110 (0xe) = True
 *
 * See src/main/java/org/liquidplayer/javascript/JNIJSValue.java for Java mirror
 */
#define ODDBALL_UNDEFINED 0x2
#define ODDBALL_NULL 0x6
#define ODDBALL_FALSE 0xa
#define ODDBALL_TRUE 0xe

#define CANPRIMITIVE(x) ((x&3)==0)
#define TOPTR(x) ((reinterpret_cast<long>(x)&~3)+1)
#define TOOBJPTR(x) ((reinterpret_cast<long>(x)&~3)+3)
#define TOSHAREDWRAP(x) (reinterpret_cast<SharedWrap<JSValue>*>(((long)x&~3)))
#define ISPOINTER(x) ((x&1)==1)
#define ISODDBALL(x) ((x&3)==2)

template<>
inline jlong SharedWrap<JSValue>::New(boost::shared_ptr<JSValue> shared) {
    if (!shared) return 0L;
    if (shared->IsObject()) {
        auto thiz = new SharedWrap<JSValue>(shared);
        return TOOBJPTR(thiz);
    } else {
        jlong reference =
                (shared->IsUndefined()) ? ODDBALL_UNDEFINED :
                (shared->IsNull()) ? ODDBALL_NULL :
                (shared->IsBoolean() && shared->IsTrue()) ? ODDBALL_TRUE :
                (shared->IsBoolean()) ? ODDBALL_FALSE : -1;
        if (reference == -1 && shared->IsNumber()) {
            double v = shared->NumberValue();
            jlong *pv = (jlong *) &v;
            if (CANPRIMITIVE(*pv)) {
                return *pv;
            }
        }
        auto thiz = new SharedWrap<JSValue>(shared);
        return TOPTR(thiz);
    }
}

template<>
inline boost::shared_ptr<JSValue> SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext> context,
                                                              jlong thiz)
{
    if (ISPOINTER(thiz)) {
        auto wrap = TOSHAREDWRAP(thiz);
        return wrap->m_shared;
    }

    Isolate::Scope isolate_scope_(Isolate::GetCurrent());
    HandleScope handle_scope_(Isolate::GetCurrent());

    Local<v8::Value> value;

    if (ISODDBALL(thiz)) {
        switch (thiz) {
            case ODDBALL_FALSE:     value = False(Isolate::GetCurrent()); break;
            case ODDBALL_TRUE:      value = True (Isolate::GetCurrent()); break;
            case ODDBALL_UNDEFINED: value = Undefined(Isolate::GetCurrent()); break;
            case ODDBALL_NULL:      value = Null(Isolate::GetCurrent());; break;
            default: break;
        }
    } else {
        double dval = * (double *) &thiz;
        value = Number::New(Isolate::GetCurrent(), dval);
    }

    return JSValue::New(context, value);
}
template<>
inline void SharedWrap<JSValue>::Dispose(jlong reference)
{
    if (ISPOINTER(reference)) {
        auto wrap = TOSHAREDWRAP(reference);
        s_zombies.push(wrap);
    } else {
        __android_log_assert("FAIL", "SharedWrap<JSValue>::~", "Attempting to dispose a primitive");
    }
}

template<>
inline long SharedWrap<JSValue>::CanonicalReference(jlong reference)
{
    if (ISPOINTER(reference)) {
        auto wrap = TOSHAREDWRAP(reference);
        boost::shared_ptr<JSValue> shared = wrap->m_shared;
        if (!shared) {
            __android_log_assert("FAIL", "SharedWrap", "CanonicalReference failed");
        }
        return reinterpret_cast<long>(&* shared);
    } else {
        return reference;
    }
}

#endif //LIQUIDCORE_SHAREDWRAP_H
