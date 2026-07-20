#!/bin/bash
set -euo pipefail

rm -rf release
mkdir -p release
cd release

cmake .. \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . -j"$(nproc)"

if [[ ! -f "dungeon" ]]; then
    echo "FAILED"
    exit 1
fi

rm -f assets world

cp -r ../assets .
cp -r ../world .

rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile

echo "SUCCESS"
