require "json"

lcpackage = JSON.parse(File.read(File.join(__dir__, "package.json")))
version = lcpackage['version']

Pod::Spec.new do |s|
  s.name = "LiquidCore-headers"
  s.version = version
  s.summary = "Header include files for LiquidCore."
  s.description = <<-DESC
Header include files for LiquidCore.  To use in a native addon, add "${PODS_CONFIGURATION_BUILD_DIR}/LiquidCore-headers/LiquidCore_headers.framework/PrivateHeaders" to your header search path.
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
    "LiquidCore/src/ios/gen/include/**/*.h",
    "LiquidCore/src/ios/header-dummy.cc"
  s.private_header_files = [
    "LiquidCore/src/ios/gen/include/*.h",
    "LiquidCore/src/ios/gen/include/**/*.h",
  ]
  s.header_mappings_dir = 'LiquidCore/src/ios/gen/include'
  s.xcconfig = {
    :CLANG_WARN_DOCUMENTATION_COMMENTS => 'NO',
    :HEADER_SEARCH_PATHS => "$(PODS_TARGET_SRCROOT)/LiquidCore/src/ios/gen/include"
  }
end
