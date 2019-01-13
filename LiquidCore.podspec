#  Test using: `pod spec lint LiquidCore.podspec` or `pod lib lint LiquidCore.podspec`.
#  valid spec and to remove all comments including this before submitting the spec.
#
#  To learn more about Podspec attributes see http://docs.cocoapods.org/specification.html
#  To see working Podspecs in the CocoaPods repo see https://github.com/CocoaPods/Specs/
#

Pod::Spec.new do |s|
  s.name = "LiquidCore"
  s.version = "0.5.1"
  s.summary = "Provides Node.js virtual machines to run inside iOS apps."
  s.description = <<-DESC
LiquidCore enables Node.js virtual machines to run inside Android and iOS apps. It provides a complete runtime environment, including a virtual file system and native SQLite3 support.
  DESC

  s.homepage = "https://github.com/LiquidPlayer/LiquidCore"
  s.license = {:type => "MIT", :file => "LICENSE.md"}

  s.author = {"Eric Lange" => "eric@flicket.tv"}

  s.platform = :ios, '10.0'

  s.source = {:git => "https://github.com/LiquidPlayer/LiquidCore.git", :tag => "#{s.version}"}

  s.exclude_files = [
      "deps/node-8.9.3/deps/cares/src/ares_iphlpapi.h",
      "deps/node-8.9.3/deps/cares/src/config-win32.h",
      "deps/node-8.9.3/deps/cares/src/ares_platform.c",
      "deps/node-8.9.3/deps/cares/src/windows_port.c",
  ]

  s.source_files = "deps/nan-2.5.1/*.h",
      #"deps/node-8.9.3/**/*",
      #"deps/node-sqlite3/**/*",
      #"deps/openssl-1.0.2o/**/*",

      "deps/node-8.9.3/deps/cares/config/darwin/ares_config.h",
      "deps/node-8.9.3/deps/cares/include/ares.h",
      "deps/node-8.9.3/deps/cares/include/ares_rules.h",
      "deps/node-8.9.3/deps/cares/include/ares_version.h",
      "deps/node-8.9.3/deps/cares/include/nameser.h",

      "deps/node-8.9.3/deps/cares/src/*.{c,h}",
      "deps/node-8.9.3/deps/http_parser/http_parser.{c,h}",

      "deps/node-8.9.3/deps/nghttp2/lib/**/*.h",
      "deps/node-8.9.3/deps/nghttp2/lib/*.c",

      "deps/node-8.9.3/deps/uv/include/pthread-barrier.h",
      "deps/node-8.9.3/deps/uv/include/tree.h",
      "deps/node-8.9.3/deps/uv/include/uv-darwin.h",
      "deps/node-8.9.3/deps/uv/include/uv-errno.h",
      "deps/node-8.9.3/deps/uv/include/uv-threadpool.h",
      "deps/node-8.9.3/deps/uv/include/uv-unix.h",
      "deps/node-8.9.3/deps/uv/include/uv-version.h",
      "deps/node-8.9.3/deps/uv/include/uv.h",

      "deps/node-8.9.3/deps/uv/src/heap-inl.h",
      "deps/node-8.9.3/deps/uv/src/queue.h",
      "deps/node-8.9.3/deps/uv/src/unix/atomic-ops.h",
      "deps/node-8.9.3/deps/uv/src/unix/internal.h",
      "deps/node-8.9.3/deps/uv/src/unix/spinlock.h",
      "deps/node-8.9.3/deps/uv/src/uv-common.h",
      "deps/node-8.9.3/deps/uv/src/*.c",

      "deps/node-8.9.3/deps/uv/src/unix/async.c",
      "deps/node-8.9.3/deps/uv/src/unix/bsd-ifaddrs.c",
      "deps/node-8.9.3/deps/uv/src/unix/core.c",
      "deps/node-8.9.3/deps/uv/src/unix/darwin-proctitle.c",
      "deps/node-8.9.3/deps/uv/src/unix/darwin.c",
      "deps/node-8.9.3/deps/uv/src/unix/dl.c",
      "deps/node-8.9.3/deps/uv/src/unix/fs.c",
      "deps/node-8.9.3/deps/uv/src/unix/fsevents.c",
      "deps/node-8.9.3/deps/uv/src/unix/getaddrinfo.c",
      "deps/node-8.9.3/deps/uv/src/unix/getnameinfo.c",
      "deps/node-8.9.3/deps/uv/src/unix/kqueue.c",
      "deps/node-8.9.3/deps/uv/src/unix/loop-watcher.c",
      "deps/node-8.9.3/deps/uv/src/unix/loop.c",
      "deps/node-8.9.3/deps/uv/src/unix/pipe.c",
      "deps/node-8.9.3/deps/uv/src/unix/poll.c",
      "deps/node-8.9.3/deps/uv/src/unix/process.c",
      "deps/node-8.9.3/deps/uv/src/unix/proctitle.c",
      "deps/node-8.9.3/deps/uv/src/unix/signal.c",
      "deps/node-8.9.3/deps/uv/src/unix/stream.c",
      "deps/node-8.9.3/deps/uv/src/unix/tcp.c",
      "deps/node-8.9.3/deps/uv/src/unix/thread.c",
      "deps/node-8.9.3/deps/uv/src/unix/timer.c",
      "deps/node-8.9.3/deps/uv/src/unix/tty.c",
      "deps/node-8.9.3/deps/uv/src/unix/udp.c",

      "deps/node-8.9.3/deps/v8/src/assert-scope.cc",
      "deps/node-8.9.3/deps/v8/src/base/bits.cc",
      "deps/node-8.9.3/deps/v8/src/base/cpu.cc",
      "deps/node-8.9.3/deps/v8/src/base/debug/stack_trace.cc",
      "deps/node-8.9.3/deps/v8/src/base/debug/stack_trace_posix.cc",
      "deps/node-8.9.3/deps/v8/src/base/division-by-constant.cc",
      "deps/node-8.9.3/deps/v8/src/base/file-utils.cc",
      "deps/node-8.9.3/deps/v8/src/base/functional.cc",
      "deps/node-8.9.3/deps/v8/src/base/ieee754.cc",
      "deps/node-8.9.3/deps/v8/src/base/logging.cc",
      "deps/node-8.9.3/deps/v8/src/base/once.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/condition-variable.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/mutex.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/platform-macos.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/platform-posix-time.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/platform-posix.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/semaphore.cc",
      "deps/node-8.9.3/deps/v8/src/base/platform/time.cc",
      "deps/node-8.9.3/deps/v8/src/base/sys-info.cc",
      "deps/node-8.9.3/deps/v8/src/base/utils/random-number-generator.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/default-platform.{cc,h}",
      "deps/node-8.9.3/deps/v8/src/libplatform/task-queue.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/tracing/trace-buffer.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/tracing/trace-config.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/tracing/trace-object.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/tracing/trace-writer.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/tracing/tracing-controller.cc",
      "deps/node-8.9.3/deps/v8/src/libplatform/worker-thread.{cc,h}",
      "deps/node-8.9.3/deps/v8/src/ostreams.cc",

      "deps/node-8.9.3/src/aliased_buffer.h",
      "deps/node-8.9.3/src/async-wrap-inl.h",
      "deps/node-8.9.3/src/async-wrap.h",
      "deps/node-8.9.3/src/base-object-inl.h",
      "deps/node-8.9.3/src/base-object.h",
      "deps/node-8.9.3/src/connect_wrap.h",
      "deps/node-8.9.3/src/connection_wrap.h",
      "deps/node-8.9.3/src/env-inl.h",
      "deps/node-8.9.3/src/env.h",
      "deps/node-8.9.3/src/handle_wrap.h",
      "deps/node-8.9.3/src/js_stream.h",
      "deps/node-8.9.3/src/module_wrap.h",
      "deps/node-8.9.3/src/node.h",
      "deps/node-8.9.3/src/node_api.h",
      "deps/node-8.9.3/src/node_api_types.h",
      "deps/node-8.9.3/src/node_buffer.h",
      "deps/node-8.9.3/src/node_constants.h",
      "deps/node-8.9.3/src/node_crypto.h",
      "deps/node-8.9.3/src/node_crypto_bio.h",
      "deps/node-8.9.3/src/node_crypto_clienthello-inl.h",
      "deps/node-8.9.3/src/node_crypto_clienthello.h",
      "deps/node-8.9.3/src/node_crypto_groups.h",
      "deps/node-8.9.3/src/node_debug_options.h",
      "deps/node-8.9.3/src/node_dtrace.h",
      "deps/node-8.9.3/src/node_http2.h",
      "deps/node-8.9.3/src/node_http2_state.h",
      "deps/node-8.9.3/src/node_i18n.h",
      "deps/node-8.9.3/src/node_internals.h",
      "deps/node-8.9.3/src/node_javascript.h",
      "deps/node-8.9.3/src/node_mutex.h",
      "deps/node-8.9.3/src/node_perf.h",
      "deps/node-8.9.3/src/node_perf_common.h",
      "deps/node-8.9.3/src/node_platform.h",
      "deps/node-8.9.3/src/node_revert.h",
      "deps/node-8.9.3/src/node_root_certs.h",
      "deps/node-8.9.3/src/node_stat_watcher.h",
      "deps/node-8.9.3/src/node_url.h",
      "deps/node-8.9.3/src/node_version.h",
      "deps/node-8.9.3/src/node_watchdog.h",
      "deps/node-8.9.3/src/node_wrap.h",
      "deps/node-8.9.3/src/pipe_wrap.h",
      "deps/node-8.9.3/src/req-wrap-inl.h",
      "deps/node-8.9.3/src/req-wrap.h",
      "deps/node-8.9.3/src/spawn_sync.h",
      "deps/node-8.9.3/src/stream_base-inl.h",
      "deps/node-8.9.3/src/stream_base.h",
      "deps/node-8.9.3/src/stream_wrap.h",
      "deps/node-8.9.3/src/string_bytes.h",
      "deps/node-8.9.3/src/string_search.h",
      "deps/node-8.9.3/src/tcp_wrap.h",
      "deps/node-8.9.3/src/tls_wrap.h",
      "deps/node-8.9.3/src/tracing/agent.h",
      "deps/node-8.9.3/src/tracing/node_trace_buffer.h",
      "deps/node-8.9.3/src/tracing/node_trace_writer.h",
      "deps/node-8.9.3/src/tracing/trace_event.h",
      "deps/node-8.9.3/src/tracing/trace_event_common.h",
      "deps/node-8.9.3/src/tty_wrap.h",
      "deps/node-8.9.3/src/udp_wrap.h",
      "deps/node-8.9.3/src/util-inl.h",
      "deps/node-8.9.3/src/util.h",

      "deps/node-8.9.3/src/async-wrap.cc",
      "deps/node-8.9.3/src/backtrace_posix.cc",
      "deps/node-8.9.3/src/cares_wrap.cc",
      "deps/node-8.9.3/src/connect_wrap.cc",
      "deps/node-8.9.3/src/connection_wrap.cc",
      "deps/node-8.9.3/src/env.cc",
      "deps/node-8.9.3/src/fs_event_wrap.cc",
      "deps/node-8.9.3/src/handle_wrap.cc",
      "deps/node-8.9.3/src/js_stream.cc",
      "deps/node-8.9.3/src/module_wrap.cc",
      "deps/node-8.9.3/src/node.cc",
      "deps/node-8.9.3/src/node_api.cc",
      "deps/node-8.9.3/src/node_buffer.cc",
      "deps/node-8.9.3/src/node_config.cc",
      "deps/node-8.9.3/src/node_constants.cc",
      "deps/node-8.9.3/src/node_contextify.cc",
      "deps/node-8.9.3/src/node_crypto.cc",
      "deps/node-8.9.3/src/node_crypto_bio.cc",
      "deps/node-8.9.3/src/node_crypto_clienthello.cc",
      "deps/node-8.9.3/src/node_debug_options.cc",
      "deps/node-8.9.3/src/node_dtrace.cc",
      "deps/node-8.9.3/src/node_http2.cc",
      "deps/node-8.9.3/src/node_http_parser.cc",
      "deps/node-8.9.3/src/node_i18n.cc",
      "deps/node-8.9.3/src/node_main.cc",
      "deps/node-8.9.3/src/node_os.cc",
      "deps/node-8.9.3/src/node_perf.cc",
      "deps/node-8.9.3/src/node_platform.cc",
      "deps/node-8.9.3/src/node_serdes.cc",
      "deps/node-8.9.3/src/node_stat_watcher.cc",
      "deps/node-8.9.3/src/node_url.cc",
      "deps/node-8.9.3/src/node_util.cc",
      "deps/node-8.9.3/src/node_v8.cc",
      "deps/node-8.9.3/src/node_watchdog.cc",
      "deps/node-8.9.3/src/node_zlib.cc",
      "deps/node-8.9.3/src/pipe_wrap.cc",
      "deps/node-8.9.3/src/signal_wrap.cc",
      "deps/node-8.9.3/src/spawn_sync.cc",
      "deps/node-8.9.3/src/stream_base.cc",
      "deps/node-8.9.3/src/stream_wrap.cc",
      "deps/node-8.9.3/src/string_bytes.cc",
      "deps/node-8.9.3/src/string_search.cc",
      "deps/node-8.9.3/src/tcp_wrap.cc",
      "deps/node-8.9.3/src/timer_wrap.cc",
      "deps/node-8.9.3/src/tls_wrap.cc",
      "deps/node-8.9.3/src/tracing/agent.cc",
      "deps/node-8.9.3/src/tracing/node_trace_buffer.cc",
      "deps/node-8.9.3/src/tracing/node_trace_writer.cc",
      "deps/node-8.9.3/src/tracing/trace_event.cc",
      "deps/node-8.9.3/src/tty_wrap.cc",
      "deps/node-8.9.3/src/udp_wrap.cc",
      "deps/node-8.9.3/src/util.cc",
      "deps/node-8.9.3/src/uv.cc",

      "deps/node-sqlite3/src/*",

      # "deps/node-8.9.3/deps/v8/src/extensions/gc-extension.cc",
      # "deps/node-8.9.3/deps/v8/src/flags.cc",
      # "deps/node-8.9.3/deps/v8/test/cctest/print-extension.cc",
      # "deps/node-8.9.3/deps/v8/test/cctest/profiler-extension.cc",

      # V82JS
      "LiquidCoreiOS/LiquidCore/node-8.9.3/V82JSC/**/*.{cpp,h}",
      #"deps/node-8.9.3/deps/v8/src/base/debug/stack_trace_posix.cc",
      #"deps/node-8.9.3/deps/v8/src/base/debug/stack_trace.{cc,h}",
      #"deps/node-8.9.3/deps/v8/src/base/*.cc",
      #"deps/node-8.9.3/deps/v8/src/assert-scope.{cc,h}",
      #"deps/node-8.9.3/deps/v8/src/base/utils/random-number-generator.{cc,h}",

      # Dynamically generated files.
      "LiquidCoreiOS/LiquidCore/node-8.9.3/V82JSC/polyfill/polyfill.c",
      "LiquidCoreiOS/LiquidCore/node-8.9.3/node/node_javascript.cc",
      "LiquidCoreiOS/LiquidCore/node-8.9.3/node/node_provider.h",

      # OTHER_CPLUSPLUSFLAGS includes
      #"deps/node-8.9.3/deps/v8/include/**/*.h",
      #"deps/node-8.9.3/deps/v8/**/*.h",

      "LiquidCoreCommon/node/*",

      # TODO export ConsoleView.xib
      "LiquidCoreiOS/LiquidCore/ConsoleView/AnsiConsoleOutputStream.h",
      "LiquidCoreiOS/LiquidCore/ConsoleView/AnsiConsoleTextView.h",
      "LiquidCoreiOS/LiquidCore/ConsoleView/CommandTextField.h",
      "LiquidCoreiOS/LiquidCore/ConsoleView/ConsoleView.h",
      "LiquidCoreiOS/LiquidCore/ConsoleView/LCConsoleSurfaceView.h",

      "LiquidCoreiOS/LiquidCore/LiquidCore/*.h",
      "LiquidCoreiOS/LiquidCore/API/*.{h,m,cpp}"

  s.public_header_files = "LiquidCoreiOS/LiquidCore/LiquidCore/*.h"

  # ――― Project Settings ――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  If your library depends on compiler flags you can set them in the xcconfig hash
  #  where they will only apply to your library. If you depend on other Podspecs
  #  you can include multiple dependencies to ensure it works.

  s.requires_arc = true

  s.libraries = ['sqlite3', # libsqlite3.tbd
                 'z' # libz.tbd
  ]
  s.vendored_libraries = "deps/openssl-1.0.2o/lib-ios/libcrypto.a", "deps/openssl-1.0.2o/lib-ios/libssl.a"
  s.frameworks = "JavaScriptCore"

  s.script_phases = [
      {
          :name => 'Generate node_javascript.cc',
          :script => '${PODS_TARGET_SRCROOT}/LiquidCoreiOS/scripts/generate_node_javascript.sh',
          :execution_position => :before_compile
      },
      {
          :name => 'Generate node_provider.h',
          :script => '${PODS_TARGET_SRCROOT}/LiquidCoreiOS/scripts/generate_node_provider.sh',
          :execution_position => :before_compile
      },
      {
          :name => 'Generate V82JSC polyfills',
          :script => '${PODS_TARGET_SRCROOT}/LiquidCoreiOS/scripts/generate_javascript_polyfills.sh',
          :execution_position => :before_compile
      },
  ]

  # Flags to pass to the compile sources.
  #s.compiler_flags = ''
  s.xcconfig = {
      # System Header Search Paths
      :HEADER_SEARCH_PATHS => [
          # OTHER_CPLUSPLUSFLAGS -I includes
          "$(PODS_TARGET_SRCROOT)/deps/node-8.9.3/deps/v8/include", "${PODS_TARGET_SRCROOT}/deps/node-8.9.3/deps/v8",
          # Other includes.
          "$(PODS_TARGET_SRCROOT)/deps/openssl-1.0.2o/include-ios",
          "$(PODS_TARGET_SRCROOT)/deps/node-8.9.3/src",
          "$(PODS_TARGET_SRCROOT)/deps/node-8.9.3/deps/nghttp2/lib/includes",
          "$(PODS_TARGET_SRCROOT)/LiquidCoreiOS/LiquidCore/node-8.9.3/node"
      ].join(' '),
      #:LIBRARY_SEARCH_PATHS => '"${PODS_CONFIGURATION_BUILD_DIR}/LiquidCore/deps/openssl-1.0.2o/include-ios"',
      :OTHER_CFLAGS => '-D_DARWIN_USE_64_BIT_INODE=1 -D_DARWIN_UNLIMITED_SELECT=1 -DHTTP_PARSER_STRICT=0 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -DHAVE_CONFIG_H',
      #:OTHER_CPLUSPLUSFLAGS => '-I"$(PODS_ROOT)/deps/node-8.9.3/deps/v8/include" -I"${PODS_ROOT}/deps/node-8.9.3/deps/v8" -DV8_OS_MACOSX -DV8_FAST_TLS_SUPPORTED_NOXXX -DNODE_WANT_INTERNALS=1 -DNODE_PLATFORM=\"iOS\" -DNODE_ARCH=\"x64\" -DV8_DEPRECATION_WARNINGS=1 -D__POSIX__ -DBUILDING_NGHTTP2 -DNODE_USE_V8_PLATFORM=1 -DHAVE_INSPECTOR=0 -DV8_INSPECTOR_USE_STL=1 -DV8_INSPECTOR_USE_OLD_STL=1 -DHAVE_OPENSSL=1 -D__POSIX__ -DHTTP_PARSER_STRICT=0 -D_LARGEFILE_SOURCE -D_GLIBCXX_USE_C99_MATH -D_REENTRANT=1 -DNODE_USE_V8_PLATFORM=1 -DNODE_USE_V8_PLATFORM=1 -DNODE_USE_V8_PLATFORM=1 -DHAVE_INSPECTOR=0 -DV8_INSPECTOR_USE_STL=1 -DV8_INSPECTOR_USE_OLD_STL=1 -DHAVE_OPENSSL=1 -DHTTP_PARSER_STRICT=0 -D_LARGEFILE_SOURCE -D_GLIBCXX_USE_C99_MATH -D_REENTRANT=1 -DHAVE_DTRACE=1 -D_FILE_OFFSET_BITS=64 -D_DARWIN_USE_64BIT_INODE=1',
      :OTHER_CPLUSPLUSFLAGS => '-DV8_OS_MACOSX -DV8_FAST_TLS_SUPPORTED_NOXXX -DNODE_WANT_INTERNALS=1 -DNODE_PLATFORM=\"iOS\" -DNODE_ARCH=\"arm64\" -DV8_DEPRECATION_WARNINGS=1 -D__POSIX__ -DBUILDING_NGHTTP2 -DNODE_USE_V8_PLATFORM=1 -DHAVE_INSPECTOR=0 -DV8_INSPECTOR_USE_STL=1 -DV8_INSPECTOR_USE_OLD_STL=1 -DHAVE_OPENSSL=1 -D__POSIX__ -DHTTP_PARSER_STRICT=0 -D_LARGEFILE_SOURCE -D_GLIBCXX_USE_C99_MATH -D_REENTRANT=1 -DNODE_USE_V8_PLATFORM=1 -DNODE_USE_V8_PLATFORM=1 -DNODE_USE_V8_PLATFORM=1 -DHAVE_INSPECTOR=0 -DV8_INSPECTOR_USE_STL=1 -DV8_INSPECTOR_USE_OLD_STL=1 -DHAVE_OPENSSL=1 -DHTTP_PARSER_STRICT=0 -D_LARGEFILE_SOURCE -D_GLIBCXX_USE_C99_MATH -D_REENTRANT=1 -DHAVE_DTRACE=1 -D_FILE_OFFSET_BITS=64 -D_DARWIN_USE_64BIT_INODE=1',

      #:CLANG_CXX_LANGUAGE_STANDARD => "gnu++14",
      #:CLANG_CXX_LIBRARY => "libc++",
      #:CLANG_ENABLE_MODULES => 'YES',
      #:CLANG_ENABLE_OBJC_ARC => 'YES',
      # Address 'LiquidCoreiOS/LiquidCore/API/Process.m:107:1':
      # 'Cannot synthesize weak property in file using manual reference counting'
      #:CLANG_ENABLE_OBJC_WEAK => 'YES',

      # Disable warnings
      :CLANG_WARN_COMMA => 'NO',
      :CLANG_WARN_DOCUMENTATION_COMMENTS => 'NO',
      :GCC_WARN_64_TO_32_BIT_CONVERSION => 'NO',
      :GCC_WARN_ABOUT_DEPRECATED_FUNCTIONS => 'NO',
      :GCC_WARN_UNUSED_VARIABLE => 'NO',
      :CLANG_WARN_UNREACHABLE_CODE => 'NO',
  }
  # Hack to set the architecture type.
  s.pod_target_xcconfig = {
      #:ONLY_ACTIVE_ARCH => 'YES',
      #:ARCHS => 'arm64',
      #:ARCHS_STANDARD => 'arm64'
  }

  s.swift_version = '3.0'

  s.resources = [
      'LiquidCoreiOS/LiquidCore/ConsoleView/ConsoleView.xib',
      'LiquidCoreiOS/LiquidCore/node_modules'
  ]

end
