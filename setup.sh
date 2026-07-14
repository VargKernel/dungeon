#!/bin/bash
set -e

sudo apt update

sudo apt install -y build-essential cmake pkg-config wget tar

sudo apt install -y libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

sudo apt install -y mingw-w64

echo "OK: SDL2 installed wia apt"

mkdir -p win_libs
cd win_libs

wget -nc https://github.com/libsdl-org/SDL/releases/download/release-2.30.1/SDL2-devel-2.30.1-mingw.tar.gz
tar -xf SDL2-devel-2.30.1-mingw.tar.gz

wget -nc https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-devel-2.8.2-mingw.tar.gz
tar -xf SDL2_image-devel-2.8.2-mingw.tar.gz

wget -nc https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-devel-2.22.0-mingw.tar.gz
tar -xf SDL2_ttf-devel-2.22.0-mingw.tar.gz

mkdir -p x86_64

cp -r SDL2-*/x86_64-w64-mingw32/* x86_64/
cp -r SDL2_image-*/x86_64-w64-mingw32/* x86_64/
cp -r SDL2_ttf-*/x86_64-w64-mingw32/* x86_64/

echo "OK: SDL2 (MinGW) installed win_libs/x86_64"
