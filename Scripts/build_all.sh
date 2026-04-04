#!/usr/bin/env bash
# build_all.sh — Linux/macOS CMake build script for NovaForge.
# Usage: ./Scripts/build_all.sh [--tests] [--server] [--release]

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

BUILD_DIR="build"
BUILD_TYPE="Debug"
BUILD_TESTS="OFF"
BUILD_SERVER="OFF"

for arg in "$@"; do
    case "$arg" in
        --tests)   BUILD_TESTS="ON" ;;
        --server)  BUILD_SERVER="ON" ;;
        --release) BUILD_TYPE="Release" ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [--tests] [--server] [--release]"
            exit 1
            ;;
    esac
done

echo "=== NovaForge Build ==="
echo "  Build type : $BUILD_TYPE"
echo "  Tests      : $BUILD_TESTS"
echo "  Server     : $BUILD_SERVER"
echo ""

cmake -B "$BUILD_DIR" \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DNF_BUILD_EDITOR=ON \
    -DNF_BUILD_GAME=ON \
    -DNF_BUILD_TESTS="$BUILD_TESTS" \
    -DNF_BUILD_SERVER="$BUILD_SERVER"

cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

if [ "$BUILD_TESTS" = "ON" ]; then
    echo ""
    echo "=== Running Tests ==="
    "$REPO_ROOT/Nova_0.1.0/bin/NFTests"
fi

echo ""
echo "=== Build complete ==="
