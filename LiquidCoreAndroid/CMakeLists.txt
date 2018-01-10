cmake_minimum_required(VERSION 3.4.1)

add_library( # Specifies the name of the library.
             liquidcore

             # Sets the library as a shared library.
             SHARED

             # External dependencies
             ../deps/node-sqlite3/src/database.cc
             ../deps/node-sqlite3/src/node_sqlite3.cc
             ../deps/node-sqlite3/src/statement.cc
             ../deps/jscshim/JSCShim.cpp

             # Common V8 objects for JNI/JSC
             src/main/cpp/Common/ContextGroup.cpp
             src/main/cpp/Common/JSContext.cpp
             src/main/cpp/Common/JSValue.cpp
             src/main/cpp/Common/LoopPreserver.cpp

             # JNI API
             src/main/cpp/JNI/JNI_JSContext.cpp
             src/main/cpp/JNI/JNI_JSObject.cpp
             src/main/cpp/JNI/JNI_JSValue.cpp
             src/main/cpp/JNI/JSFunction.cpp
             src/main/cpp/JNI/SharedWrap.cpp

             # JavaScriptCore -> V8 bridge
             src/main/cpp/JSC/JSC_JSBase.cpp
             src/main/cpp/JSC/JSC_JSContext.cpp
             src/main/cpp/JSC/JSC_JSObject.cpp
             src/main/cpp/JSC/JSC_JSString.cpp
             src/main/cpp/JSC/JSC_JSValue.cpp
             src/main/cpp/JSC/ObjectData.cpp
             src/main/cpp/JSC/OpaqueJSClass.cpp
             src/main/cpp/JSC/OpaqueJSContext.cpp
             src/main/cpp/JSC/OpaqueJSContextGroup.cpp
             src/main/cpp/JSC/OpaqueJSString.cpp
             src/main/cpp/JSC/OpaqueJSValue.cpp

             # Node.js
             src/main/cpp/node/nodedroid_file.cc
             src/main/cpp/node/NodeInstance.cpp
             src/main/cpp/node/process_wrap.cc
             src/main/cpp/node/sqlite3_shim.cpp

             # Test files (FIXME: do not compile into release builds)
             src/main/cpp/androidTest/testapi.cpp
             src/main/cpp/androidTest/CustomGlobalObjectClassTest.cpp
             src/main/cpp/androidTest/JSNode.cpp
             src/main/cpp/androidTest/JSNodeList.cpp
             src/main/cpp/androidTest/Node.cpp
             src/main/cpp/androidTest/NodeList.cpp
             src/main/cpp/androidTest/minidom.cpp

             )

add_library( node
             SHARED
             IMPORTED )

set_target_properties(
             # Specifies the target library.
             node

             # Specifies the parameter you want to define.
             PROPERTIES IMPORTED_LOCATION

             # Provides the path to the library you want to import.
             ${PROJECT_SOURCE_DIR}/../deps/node-8.9.3/prebuilt/${ANDROID_ABI}/libnode.so )

include_directories(
             src/main/cpp
             src/../../deps/JavaScriptCore/include
             src/../../deps/node-8.9.3/src
             src/../../deps/node-8.9.3/deps/v8
             src/../../deps/node-8.9.3/deps/v8/include
             src/../../deps/node-8.9.3/deps/uv/include
             src/../../deps/node-8.9.3/deps/cares/include
             src/../../deps/node-8.9.3/deps/openssl/openssl/include
             src/../../deps/node-8.9.3/deps/http_parser
             src/../../deps/utfcpp
             src/../../deps/sqlite-autoconf-3150000
             src/../../deps/nan-2.5.1
             )

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

unset(DEFS_RELEASE)
unset(CFLAGS_RELEASE)

list (APPEND DEFS_RELEASE
	-DNODE_ARCH="${NODE_ARCH}"
	-DNODE_PLATFORM="android"
	-DNODE_WANT_INTERNALS=1
	-DV8_DEPRECATION_WARNINGS=1
	-DNODE_SHARED_MODE
	-DNODE_USE_V8_PLATFORM=1
	-DHAVE_INSPECTOR=0
	-DV8_INSPECTOR_USE_STL=1
	-DV8_INSPECTOR_USE_OLD_STL=1
	-DHAVE_OPENSSL=1
	-D__POSIX__
	-DHTTP_PARSER_STRICT=0
	-D_LARGEFILE_SOURCE
	-D_GLIBCXX_USE_C99_MATH
    -D_REENTRANT=1
    -DSQLITE_DISABLE_LFS
    -DSQLITE_THREADSAFE=1
    -DSQLITE_ENABLE_FTS3
    -DSQLITE_ENABLE_FTS4
    -DSQLITE_ENABLE_FTS5
    -DSQLITE_ENABLE_JSON1
    -DSQLITE_ENABLE_RTREE
    -DJSCSHIM_IMPL
    )
    
list (APPEND CFLAGS_RELEASE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -fPIC
    -O3
    -fno-omit-frame-pointer
    -fPIE
    -Wno-strict-aliasing
    -Wno-unused-variable
    -Wno-mismatched-tags
    -Wno-unused-result
    -fno-rtti
    )

string (REPLACE ";" " " CPP_FLAGS_STR "${CFLAGS_RELEASE} ${DEFS_RELEASE}")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CPP_FLAGS_STR}" )

target_link_libraries( liquidcore node ${log-lib} )
