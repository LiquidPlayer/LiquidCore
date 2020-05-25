/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 *  strict
 * @format
 */
"use strict";

const defaultCreateModuleIdFactory = require("metro/src/lib/createModuleIdFactory");

exports.assetExts = [
  // Image formats
  "bmp",
  "gif",
  "jpg",
  "jpeg",
  "png",
  "psd",
  "svg",
  "webp", // Video formats
  "m4v",
  "mov",
  "mp4",
  "mpeg",
  "mpg",
  "webm", // Audio formats
  "aac",
  "aiff",
  "caf",
  "m4a",
  "mp3",
  "wav", // Document formats
  "html",
  "json",
  "pdf",
  "yaml",
  "yml", // Font formats
  "otf",
  "ttf", // Archives (virtual files)
  "zip"
];
exports.sourceExts = ["js", "json", "ts", "tsx"];
// LiquidCore: Override default require
//exports.moduleSystem = require.resolve("metro/src/lib/polyfills/require.js");
try {
  exports.moduleSystem = require.resolve("../../../../lib/metro-polyfill/require.js");
} catch (e) {
  const path=require('path')
  const local_path = path.resolve('.') + '/node_modules/';
  exports.moduleSystem = require.resolve(local_path+"liquidcore/lib/metro-polyfill/require.js");
}
exports.platforms = ["ios", "android", "windows", "web"];
exports.DEFAULT_METRO_MINIFIER_PATH = "metro-minify-uglify";
exports.defaultCreateModuleIdFactory = defaultCreateModuleIdFactory;
