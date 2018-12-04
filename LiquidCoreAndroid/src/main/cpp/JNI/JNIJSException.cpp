/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "JNIJSException.h"

jmethodID JNIJSException::m_cid = 0;
jclass JNIJSException::m_clazz = 0;
