/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
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
    SharedWrapBase() = default;
    static Queue<SharedWrapBase*> s_zombies;
    static void FreeZombiesThread();
};

template<typename T>
class SharedWrap : SharedWrapBase {
public:
    inline static jlong New(const boost::shared_ptr<T>& shared)
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
    inline static boost::shared_ptr<T> Shared(const boost::shared_ptr<JSContext>& context, jlong thiz)
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
    ~SharedWrap()
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
    explicit SharedWrap(boost::shared_ptr<T> shared)
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

#define CANPRIMITIVE(x) (((unsigned long)(x)&0x3UL)==0UL)
#define TOPTR(x) ((reinterpret_cast<unsigned long>(x)&~0x3UL)+0x1UL)
#define TOOBJPTR(x) ((reinterpret_cast<unsigned long>(x)&~0x3UL)+0x3UL)
#define TOSHAREDWRAP(x) (reinterpret_cast<SharedWrap<JSValue>*>(((unsigned long)(x)&~0x3UL)))
#define ISPOINTER(x) (((unsigned long)(x)&0x1UL)==0x1UL)
#define ISODDBALL(x) (((unsigned long)(x)&0x3UL)==0x2UL)

template<>
inline jlong SharedWrap<JSValue>::New(const boost::shared_ptr<JSValue>& shared) {
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
            auto pv = (jlong *) &v;
            if (CANPRIMITIVE(*pv)) {
                reference = *pv;
            }
        }
        if (reference != -1) return reference;

        auto thiz = new SharedWrap<JSValue>(shared);
        return TOPTR(thiz);
    }
}

template<>
inline boost::shared_ptr<JSValue> SharedWrap<JSValue>::Shared(const boost::shared_ptr<JSContext>& context,
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
