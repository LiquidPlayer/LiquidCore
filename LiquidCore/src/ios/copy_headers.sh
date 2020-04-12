#!/usr/bin/env bash

# Resolve absolute paths.
CURRENT_DIR=$(cd $(dirname "$0"); pwd)
#PWD=`pwd`
#CURRENT_DIR="$PWD/LiquidCore/src/ios"
SOURCE_DIR=$(cd "$CURRENT_DIR/.."; pwd)
DEPS_DIR=$(cd "$CURRENT_DIR/../../../deps/node-10.15.3"; pwd)
OUTPUT_DIR="$SOURCE_DIR/ios/gen/include"

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

COPY_DIRS=(
    "$DEPS_DIR/deps/uv/include,uv"
    "$DEPS_DIR/deps/v8/include,v8"
    "$DEPS_DIR/deps/openssl/openssl/include,"
    "$SOURCE_DIR/include,"
    "$DEPS_DIR/deps/cares/include,cares"
    "$DEPS_DIR/deps/http_parser,http_parser"
    "$DEPS_DIR/deps/nghttp2/lib/includes/nghttp2,nghttp2"
    "$DEPS_DIR/src,node"
)

for INDIR in "${COPY_DIRS[@]}";
do
   IFS=","
   set -- $INDIR
   rsync -a --prune-empty-dirs --include '*/' --include '*.h' --exclude '*' "$1/" "$OUTPUT_DIR/$2/"
done
rm -rf "$OUTPUT_DIR/internal"
cp "$DEPS_DIR/deps/openssl/config/archs/darwin64-x86_64-cc/no-asm/include/openssl/opensslconf.h" "$OUTPUT_DIR/openssl"

echo "Copied headers to: $OUTPUT_DIR";