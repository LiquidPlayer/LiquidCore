#
# Android.mk
# AndroidJSCore project
#
# https://github.com/ericwlange/AndroidJSCore/
#
# Created by Eric Lange
#
#
# Copyright (c) 2014-2016 Eric Lange. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := node
LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libnode.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := nodedroid
LOCAL_SRC_FILES := JSContext.cpp \
                   JSValue.cpp \
                   JSObject.cpp \
                   NodeInstance.cpp \
                   nodedroid_file.cc \
                   process_wrap.cc

LOCAL_SHARED_LIBRARIES := libnode

DEFS_Release := \
	-DNODE_ARCH="arm" \
	-DNODE_PLATFORM="android" \
	-DNODE_WANT_INTERNALS=1 \
	-DV8_DEPRECATION_WARNINGS=1 \
	-DNODE_SHARED_MODE \
	-DNODE_USE_V8_PLATFORM=1 \
	-DHAVE_INSPECTOR=1 \
	-DV8_INSPECTOR_USE_STL=1 \
	-DV8_INSPECTOR_USE_OLD_STL=1 \
	-DHAVE_OPENSSL=1 \
	-D__POSIX__ \
	-DHTTP_PARSER_STRICT=0 \
	-D_LARGEFILE_SOURCE \
	-D_FILE_OFFSET_BITS=64 \
	-D_GLIBCXX_USE_C99_MATH

CFLAGS_Release := \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -fPIC \
    -O3 \
    -fno-omit-frame-pointer \
    -fPIE \
    -Wno-strict-aliasing \
    -Wno-unused-variable

CFLAGS_CC_Release := \
	-fno-rtti \
	-fno-exceptions \
	-std=gnu++0x

LOCAL_CPPFLAGS  := -I$(LOCAL_PATH)/include/node \
    -I$(LOCAL_PATH)/include/v8 -I$(LOCAL_PATH)/include/v8/include -I$(LOCAL_PATH)/include/uv \
    -I$(LOCAL_PATH)/include/cares -I$(LOCAL_PATH)/include/openssl \
    -I$(LOCAL_PATH)/include/http_parser $(DEFS_Release) $(CFLAGS_Release) $(CFLAGS_CC_Release)
LOCAL_LDFLAGS := -llog -lm -ldl

include $(BUILD_SHARED_LIBRARY)