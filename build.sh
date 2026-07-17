#!/bin/bash
set -euo pipefail

rm -rf build
mkdir -p build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . -j"$(nproc)"

if [[ -f "dungeon" ]]; then
    echo "SUCCESS"
else
    echo "FAILED"
    exit 1
fi
