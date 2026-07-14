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

# CMake POST_BUILD оставил assets/world как симлинки — заменяем их реальными копиями
rm -f assets world

cp -r ../assets .
cp -r ../world .

# Убираем мусор cmake, оставляя только релизные файлы
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile

echo "SUCCESS"
