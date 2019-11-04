#!/usr/bin/env bash

# Resolve absolute paths.
CURRENT_DIR=$(cd $(dirname "$0"); pwd)
SOURCE_DIR=$(cd "$CURRENT_DIR/.."; pwd)
DEPS_DIR=$(cd "$CURRENT_DIR/../../../deps/node-10.15.3"; pwd)
OUTPUT_DIR="$SOURCE_DIR/ios/gen/include"

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

COPY_DIRS=(
    "$DEPS_DIR/deps/uv/include"
    "$DEPS_DIR/deps/v8/include"
    "$DEPS_DIR/deps/openssl/openssl/include"
    "$SOURCE_DIR/include"
    "$DEPS_DIR/deps/cares/include"
    "$DEPS_DIR/deps/http_parser"
    "$DEPS_DIR/deps/nghttp2/lib/includes/nghttp2"
    "$DEPS_DIR/src"
)

for INDIR in "${COPY_DIRS[@]}";
do
   rsync -a --prune-empty-dirs --include '*/' --include '*.h' --exclude '*' "$INDIR/" "$OUTPUT_DIR/"
done
rm -rf "$OUTPUT_DIR/internal"

echo "Copied headers to: $OUTPUT_DIR";