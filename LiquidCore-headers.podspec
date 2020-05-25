require "json"

lcpackage = JSON.parse(File.read(File.join(__dir__, "package.json")))
version = lcpackage['version']

Pod::Spec.new do |s|
  s.name = "LiquidCore-headers"
  s.version = version
  s.summary = "Header include files for LiquidCore."
  s.description = <<-DESC
    Header include files for LiquidCore addons.  To use in a native addon,
    add "${PODS_CONFIGURATION_BUILD_DIR}/LiquidCore-headers/LiquidCore_headers.framework/PrivateHeaders"
    to your header search path.
  DESC
  s.homepage = "https://github.com/LiquidPlayer/LiquidCore"
  s.license = {:type => "MIT", :file => "LICENSE.md"}
  s.author = {"Eric Lange" => "eric@flicket.tv"}
  s.platform = :ios, '11.0'
  s.source = {:git => "https://github.com/LiquidPlayer/LiquidCore.git", :tag => "#{s.version}"}
  s.prepare_command = <<-CMD
    bash LiquidCore/src/ios/copy_headers.sh
  CMD
  s.source_files =
    "LiquidCore/src/ios/gen/include/*.h",
    "LiquidCore/src/ios/gen/include/uv/*.h",
    "LiquidCore/src/ios/gen/include/uv/uv/errno.h",
    "LiquidCore/src/ios/gen/include/uv/uv/aix.h",
    "LiquidCore/src/ios/gen/include/uv/uv/android-ifaddrs.h",
    "LiquidCore/src/ios/gen/include/uv/uv/bsd.h",
    "LiquidCore/src/ios/gen/include/uv/uv/darwin.h",
    "LiquidCore/src/ios/gen/include/uv/uv/linux.h",
    "LiquidCore/src/ios/gen/include/uv/uv/os390.h",
    "LiquidCore/src/ios/gen/include/uv/uv/posix.h",
    "LiquidCore/src/ios/gen/include/uv/uv/stdint-msvc2008.h",
    "LiquidCore/src/ios/gen/include/uv/uv/sunos.h",
    "LiquidCore/src/ios/gen/include/uv/uv/threadpool.h",
    "LiquidCore/src/ios/gen/include/uv/uv/tree.h",
    "LiquidCore/src/ios/gen/include/uv/uv/unix.h",
    "LiquidCore/src/ios/gen/include/uv/uv/version.h",
    "LiquidCore/src/ios/gen/include/uv/uv/win.h",
    "LiquidCore/src/ios/gen/include/node/*.h",
    "LiquidCore/src/ios/gen/include/node/inspector/**/*.h",
    "LiquidCore/src/ios/gen/include/node/large_pages/**/*.h",
    "LiquidCore/src/ios/gen/include/node/tracing/**/*.h",
    "LiquidCore/src/ios/gen/include/v8/*.h",
    "LiquidCore/src/ios/gen/include/v8/libplatform/**/*.h",
    "LiquidCore/src/ios/gen/include/openssl/**/*.h",
    "LiquidCore/src/ios/gen/include/http_parser/*.h",
    "LiquidCore/src/ios/gen/include/nghttp2/*.h",
    "LiquidCore/src/ios/gen/include/cares/*.h",
    "LiquidCore/src/ios/header-dummy.cc"
  s.private_header_files = [
    "LiquidCore/src/ios/gen/include/*.h",
    "LiquidCore/src/ios/gen/include/uv/*.h",
    "LiquidCore/src/ios/gen/include/uv/uv/errno.h",
    "LiquidCore/src/ios/gen/include/uv/uv/aix.h",
    "LiquidCore/src/ios/gen/include/uv/uv/android-ifaddrs.h",
    "LiquidCore/src/ios/gen/include/uv/uv/bsd.h",
    "LiquidCore/src/ios/gen/include/uv/uv/darwin.h",
    "LiquidCore/src/ios/gen/include/uv/uv/linux.h",
    "LiquidCore/src/ios/gen/include/uv/uv/os390.h",
    "LiquidCore/src/ios/gen/include/uv/uv/posix.h",
    "LiquidCore/src/ios/gen/include/uv/uv/stdint-msvc2008.h",
    "LiquidCore/src/ios/gen/include/uv/uv/sunos.h",
    "LiquidCore/src/ios/gen/include/uv/uv/threadpool.h",
    "LiquidCore/src/ios/gen/include/uv/uv/tree.h",
    "LiquidCore/src/ios/gen/include/uv/uv/unix.h",
    "LiquidCore/src/ios/gen/include/uv/uv/version.h",
    "LiquidCore/src/ios/gen/include/uv/uv/win.h",
    "LiquidCore/src/ios/gen/include/node/*.h",
    "LiquidCore/src/ios/gen/include/node/inspector/**/*.h",
    "LiquidCore/src/ios/gen/include/node/large_pages/**/*.h",
    "LiquidCore/src/ios/gen/include/node/tracing/**/*.h",
    "LiquidCore/src/ios/gen/include/v8/*.h",
    "LiquidCore/src/ios/gen/include/v8/libplatform/**/*.h",
    "LiquidCore/src/ios/gen/include/openssl/**/*.h",
    "LiquidCore/src/ios/gen/include/http_parser/*.h",
    "LiquidCore/src/ios/gen/include/nghttp2/*.h",
    "LiquidCore/src/ios/gen/include/cares/*.h",
  ]
  s.header_mappings_dir = "LiquidCore/src/ios/gen/include"
  s.preserve_paths = "LiquidCore/src/ios/gen/include/uv/uv/errno.h"
  s.xcconfig = {
    :CLANG_WARN_DOCUMENTATION_COMMENTS => 'NO',
    :GCC_WARN_UNUSED_FUNCTION => 'NO',
    :HEADER_SEARCH_PATHS => [
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include",
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include/uv",
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include/v8",
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include/cares",
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include/http_parser",
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include/nghttp2",
      "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include/node",
    ].join(' ')
  }
end