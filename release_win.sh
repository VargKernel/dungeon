#!/bin/bash
set -euo pipefail

rm -rf release_win
mkdir -p release_win
cd release_win

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../windows_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . -j"$(nproc)"

if [[ ! -f "dungeon.exe" ]]; then
    echo "FAILED"
    exit 1
fi

# SDL DLLs
if compgen -G "../win_libs/x86_64/bin/*.dll" > /dev/null; then
    cp -v ../win_libs/x86_64/bin/*.dll .
else
    echo "WARN: SDL DLLs not found in ../win_libs/x86_64/bin"
fi

for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    src="$(x86_64-w64-mingw32-g++ -print-file-name="$dll")"
    if [[ -f "$src" && "$src" != "$dll" ]]; then
        cp -v "$src" .
    else
        echo "WARN: runtime DLL not found: $dll"
    fi
done

rm -f assets world

cp -r ../assets .
cp -r ../world .

rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile

echo "SUCCESS"
