#!/usr/bin/env bash

# Resolve absolute paths.
CURRENT_DIR=$(cd $(dirname "$0"); pwd)
SOURCE_DIR=$(cd "$CURRENT_DIR/.."; pwd)

INPUT_FILE="$SOURCE_DIR/../deps/node-8.9.3/src/node_provider.d"
OUTPUT_FILE="$SOURCE_DIR/LiquidCore/node-8.9.3/node/node_provider.h"

cd "$SOURCE_DIR/../deps/node-8.9.3"
cp "$SOURCE_DIR/LiquidCore/node-8.9.3/node/config.gypi" .

dtrace -h -xnolibs -s "$INPUT_FILE" -o "$OUTPUT_FILE"

echo "generated: $OUTPUT_FILE";
