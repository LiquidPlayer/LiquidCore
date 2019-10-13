/*
 * Copyright (c) 2016 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef NODEDROID_JSJNI_H
#define NODEDROID_JSJNI_H

#include <android/log.h>
#include "Common/Common.h"
#include "JNI/SharedWrap.h"

#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_javascript_##package##_##f
#define PARAMS JNIEnv* env, jobject thiz
#define STATIC JNIEnv* env, jclass klass

jclass findClass(JNIEnv *env, const char* name);

#endif //NODEDROID_JSJNI_H
