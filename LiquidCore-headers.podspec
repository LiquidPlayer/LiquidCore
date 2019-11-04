Pod::Spec.new do |s|
  s.name = "LiquidCore-headers"
  s.version = "0.7.0"
  s.summary = "Provides Node.js virtual machines to run inside iOS apps."
  s.description = <<-DESC
LiquidCore enables Node.js virtual machines to run inside iOS apps. It provides a complete runtime environment, including a virtual file system.
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
