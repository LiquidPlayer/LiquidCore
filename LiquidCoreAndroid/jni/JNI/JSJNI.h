//
// Created by Eric on 11/24/16.
//

#ifndef NODEDROID_JSJNI_H
#define NODEDROID_JSJNI_H

#include "common.h"

#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_javascript_##package##_##f
#define PARAMS JNIEnv* env, jobject thiz

#endif //NODEDROID_JSJNI_H
