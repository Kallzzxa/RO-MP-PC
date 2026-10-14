#!/bin/bash
# Build RO-MP PC di Linux. Butuh: cmake, g++, git + dev package X11:
#   sudo apt install cmake g++ git libx11-dev libxrandr-dev libxinerama-dev \
#       libxcursor-dev libxi-dev mesa-common-dev
set -e
cd "$(dirname "$0")"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
echo ""
echo "OK -> ./build/romp  (atau ./build/romp.exe di Windows)"
