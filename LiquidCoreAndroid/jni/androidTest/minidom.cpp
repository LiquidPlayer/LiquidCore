/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "JavaScriptCore/JSContextRef.h"
#include "JSNode.h"
#include "JavaScriptCore/JSObjectRef.h"
#include "JavaScriptCore/JSStringRef.h"
#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include <android/log.h>
#define ASSERT(x) if(!(x)) __android_log_assert("conditional", "ASSERTION FAIL", "%s(%d): %s", __FILE__, __LINE__, #x);
#undef printf
#define printf(...) __android_log_print(ANDROID_LOG_INFO, __FILE__, __VA_ARGS__)
#undef fprintf
#define fprintf(x,...) __android_log_print(ANDROID_LOG_ERROR, __FILE__, __VA_ARGS__)
#define UNUSED_PARAM(x)

static JSValueRef print(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

extern "C" JNIEXPORT jint JNICALL Java_org_liquidplayer_test_JSC_minidom(JNIEnv* env,
    jobject thiz, jstring minidom_js, jlong group)
{
    const char *scriptPath = "minidom.js";
    const char *scriptUTF8 = env->GetStringUTFChars(minidom_js, NULL);

    JSContextGroupRef contextGroup = reinterpret_cast<JSContextGroupRef>(group);

    JSGlobalContextRef context = JSGlobalContextCreateInGroup(contextGroup, NULL);
    JSObjectRef globalObject = JSContextGetGlobalObject(context);

    JSStringRef printIString = JSStringCreateWithUTF8CString("print");
    JSObjectSetProperty(context, globalObject, printIString, JSObjectMakeFunctionWithCallback(context, printIString, print), kJSPropertyAttributeNone, NULL);
    JSStringRelease(printIString);

    JSStringRef node = JSStringCreateWithUTF8CString("Node");
    JSObjectSetProperty(context, globalObject, node, JSObjectMakeConstructor(context, JSNode_class(context), JSNode_construct), kJSPropertyAttributeNone, NULL);
    JSStringRelease(node);

    JSStringRef script = JSStringCreateWithUTF8CString(scriptUTF8);
    JSValueRef exception;
    JSValueRef result = JSEvaluateScript(context, script, NULL, NULL, 1, &exception);
    if (result)
        printf("PASS: Test script executed successfully.\n");
    else {
        printf("FAIL: Test script threw exception:\n");
        JSStringRef exceptionIString = JSValueToStringCopy(context, exception, NULL);
        size_t exceptionUTF8Size = JSStringGetMaximumUTF8CStringSize(exceptionIString);
        char* exceptionUTF8 = (char*)malloc(exceptionUTF8Size);
        JSStringGetUTF8CString(exceptionIString, exceptionUTF8, exceptionUTF8Size);
        printf("%s\n", exceptionUTF8);
        free(exceptionUTF8);
        JSStringRelease(exceptionIString);
    }
    JSStringRelease(script);

    globalObject = 0;
    JSGlobalContextRelease(context);

    env->ReleaseStringUTFChars(minidom_js, scriptUTF8);

    printf("PASS: Program exited normally.\n");
    return 0;
}

static JSValueRef print(JSContextRef context, JSObjectRef object, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    UNUSED_PARAM(object);
    UNUSED_PARAM(thisObject);

    if (argumentCount > 0) {
        JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);
        size_t numChars = JSStringGetMaximumUTF8CStringSize(string);
        char stringUTF8[numChars];
        JSStringGetUTF8CString(string, stringUTF8, numChars);
        printf("%s\n", stringUTF8);
    }

    return JSValueMakeUndefined(context);
}
