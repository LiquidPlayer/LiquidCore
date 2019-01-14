#!/usr/bin/env bash

# Resolve absolute paths.
CURRENT_DIR=$(cd $(dirname "$0"); pwd)
SOURCE_DIR=$(cd "$CURRENT_DIR/.."; pwd)

SCRIPT_OUTPUT="$SOURCE_DIR/LiquidCore/node-8.9.3/node/node_javascript.cc"
cd "$SOURCE_DIR/../deps/node-8.9.3"
cp "$SOURCE_DIR/LiquidCore/node-8.9.3/node/config.gypi" .
SCRIPT_INPUT_FILES=''

while read INFILE; do
    SCRIPT_INPUT_FILES="$SCRIPT_INPUT_FILES $INFILE"
done < "$CURRENT_DIR/generate_node_javascript_input.txt"

eval tools/js2c.py "${SCRIPT_OUTPUT}" ${SCRIPT_INPUT_FILES}

echo "generated: $SCRIPT_OUTPUT";
