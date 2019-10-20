cmake_minimum_required(VERSION 3.4.1)

set( SOURCES

     # Common V8 objects for JNI/JSC
     Common/ContextGroup.cpp
     Common/JSContext.cpp
     Common/JSValue.cpp
     Common/LoopPreserver.cpp

     # JNI API
     JNI/JNI_JSContext.cpp
     JNI/JNI_JSContextGroup.cpp
     JNI/JNI_JSObject.cpp
     JNI/JNI_JSValue.cpp
     JNI/JNI_LoopPreserver.cpp
     JNI/JNI_OnLoad.cpp
     JNI/JNIJSException.cpp
     JNI/JSFunction.cpp
     JNI/SharedWrap.cpp

     # JavaScriptCore -> V8 bridge
     JSC/JSC_JSBase.cpp
     JSC/JSC_JSContext.cpp
     JSC/JSC_JSContextGroup.cpp
     JSC/JSC_JSObject.cpp
     JSC/JSC_JSString.cpp
     JSC/JSC_JSValue.cpp
     JSC/ObjectData.cpp
     JSC/OpaqueJSClass.cpp
     JSC/OpaqueJSContext.cpp
     JSC/OpaqueJSContextGroup.cpp
     JSC/OpaqueJSString.cpp
     JSC/OpaqueJSValue.cpp

	 # libuv
	 ../../../../deps/node-10.15.3/deps/uv/src/fs-poll.c
     ../../../../deps/node-10.15.3/deps/uv/src/inet.c
     ../../../../deps/node-10.15.3/deps/uv/src/threadpool.c
     ../../../../deps/node-10.15.3/deps/uv/src/timer.c
     ../../../../deps/node-10.15.3/deps/uv/src/uv-data-getter-setters.c
     ../../../../deps/node-10.15.3/deps/uv/src/uv-common.c
     ../../../../deps/node-10.15.3/deps/uv/src/version.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/async.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/core.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/dl.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/fs.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/getaddrinfo.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/getnameinfo.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/loop.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/loop-watcher.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/pipe.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/poll.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/process.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/signal.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/stream.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/tcp.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/thread.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/tty.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/udp.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/proctitle.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/linux-core.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/linux-inotify.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/linux-syscalls.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/pthread-fixes.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/android-ifaddrs.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/procfs-exepath.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/sysinfo-loadavg.c
     ../../../../deps/node-10.15.3/deps/uv/src/unix/sysinfo-memory.c

)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set(SOURCES ${SOURCES}

     # Test files
     ../../androidTest/cpp/testapi.cpp
     ../../androidTest/cpp/CustomGlobalObjectClassTest.cpp
     ../../androidTest/cpp/JSNode.cpp
     ../../androidTest/cpp/JSNodeList.cpp
     ../../androidTest/cpp/Node.cpp
     ../../androidTest/cpp/NodeList.cpp
     ../../androidTest/cpp/minidom.cpp
     )
endif()


add_library( # Specifies the name of the library.
             liquidjs

             # Sets the library as a shared library.
             SHARED

             ${SOURCES}
             )

add_library(v8_base STATIC IMPORTED)
set_target_properties(
		v8_base PROPERTIES IMPORTED_LOCATION
		${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_base.a )
add_library(v8_libbase STATIC IMPORTED)
set_target_properties(
		v8_libbase PROPERTIES IMPORTED_LOCATION
		${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_libbase.a )
add_library(v8_snapshot STATIC IMPORTED)
set_target_properties(
		v8_snapshot PROPERTIES IMPORTED_LOCATION
		${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_snapshot.a )
add_library(v8_libsampler STATIC IMPORTED)
set_target_properties(
		v8_libsampler PROPERTIES IMPORTED_LOCATION
		${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_libsampler.a )
add_library(v8_libplatform STATIC IMPORTED)
set_target_properties(
		v8_libplatform PROPERTIES IMPORTED_LOCATION
		${PROJECT_SOURCE_DIR}/../../../prebuilt/${ANDROID_ABI}/libv8_libplatform.a )

include_directories(
             ./
             ../../../../deps/JavaScriptCore/include
             ../../../../deps/node-10.15.3/deps/v8
             ../../../../deps/node-10.15.3/deps/v8/include
             ../../../../deps/node-10.15.3/deps/uv/include
		     ../../../../deps/node-10.15.3/deps/uv/src
             ../../../../deps/utfcpp
             ../../../../deps/boost_1_66_0
             )

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

unset(DEFS_RELEASE)
unset(CFLAGS_RELEASE)

list (APPEND DEFS_RELEASE
	-DV8_DEPRECATION_WARNINGS=1
	-DNODE_USE_V8_PLATFORM=1
	-DHAVE_INSPECTOR=0
	-DV8_INSPECTOR_USE_STL=1
	-DV8_INSPECTOR_USE_OLD_STL=1
	-D__POSIX__
	-D_LARGEFILE_SOURCE
	-D_GLIBCXX_USE_C99_MATH
    -D_REENTRANT=1
    -D_FILE_OFFSET_BITS=64
    )

list (APPEND CFLAGS_RELEASE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-function
    -fPIC
    -O3
    -fno-omit-frame-pointer
    -fPIE
    -Wno-strict-aliasing
    -Wno-unused-variable
    -Wno-mismatched-tags
    -Wno-unused-result
	-Wno-sign-compare
    -Wno-implicit-function-declaration
    )

list (APPEND CPPFLAGS_RELEASE
	-fno-rtti
	-std=c++1y
	)


if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
	list (APPEND CFLAGS_RELEASE -DDEBUG)
endif()

string (REPLACE ";" " " C_FLAGS_STR "${CFLAGS_RELEASE} ${DEFS_RELEASE}")
SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} ${C_FLAGS_STR}" )

string (REPLACE ";" " " CPP_FLAGS_STR "${CFLAGS_RELEASE} ${CPPFLAGS_RELEASE} ${DEFS_RELEASE}")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CPP_FLAGS_STR}" )

target_link_libraries( liquidjs v8_libplatform v8_base v8_libbase v8_snapshot v8_libsampler ${log-lib} )
