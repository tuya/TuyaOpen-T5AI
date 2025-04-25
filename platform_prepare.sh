#!/usr/bin/env bash

set -e

TOP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo $TOP_DIR

echo "Start board prepare ..."

bash $TOP_DIR/toolchain_get.sh $TOP_DIR/../tools

echo "Run board prepare success ..."