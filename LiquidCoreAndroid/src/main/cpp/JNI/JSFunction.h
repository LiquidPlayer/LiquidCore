/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_JSFUNCTION_H
#define LIQUIDCORE_JSFUNCTION_H

#include "Common/Common.h"

using namespace v8;

class JSFunction : public JSValue {
public:
    JSFunction(JNIEnv* env, jobject thiz, boost::shared_ptr<JSContext> ctx, jstring name_);
    virtual ~JSFunction();

    void setException(boost::shared_ptr<JSValue> exception)
    {
        m_exception = exception;
    }

    static boost::shared_ptr<JSValue> New(JNIEnv* env, jobject thiz, jlong javaContext, jstring name_);

private:
    static void StaticFunctionCallback(const FunctionCallbackInfo< v8::Value > &info);
    virtual void FunctionCallback(const FunctionCallbackInfo< v8::Value > &info);

    void clearException()
    {
        m_exception = boost::shared_ptr<JSValue>();
    }

    JavaVM *m_jvm;
    jobject m_JavaThis;
    jmethodID m_constructorMid;
    jmethodID m_functionMid;
    boost::atomic_shared_ptr<JSValue> m_exception;
};

#endif //LIQUIDCORE_JSFUNCTION_H
