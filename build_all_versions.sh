#!/bin/bash

# Build script that works on both Mac and Docker
# Detects platform and uses appropriate settings

set -e

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="mac"
    CPU_COUNT=$(sysctl -n hw.ncpu)
    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-O3 -flto -funroll-loops'"
else
    PLATFORM="linux"
    CPU_COUNT=$(nproc)
    CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-O3 -flto -funroll-loops'"
fi

echo "=========================================="
echo "Building All Versions"
echo "Platform: $PLATFORM"
echo "CPU Cores: $CPU_COUNT"
echo "=========================================="
echo ""

VERSIONS=(
    "original"
    "optimized"
    "optimized_v2"
    "art"
    "art_simd"
    "event_sourcing"
    "production_basic"
    "production_fast"
    "production_safe"
)

SUCCESSFUL=()
FAILED=()

for version in "${VERSIONS[@]}"; do
    echo "----------------------------------------"
    echo "Building: $version"
    echo "----------------------------------------"
    
    VERSION_DIR="versions/$version"
    BUILD_DIR="$VERSION_DIR/build"
    
    if [ ! -d "$VERSION_DIR" ]; then
        echo "  ⚠️  Directory not found: $VERSION_DIR"
        FAILED+=("$version (not found)")
        continue
    fi
    
    if [ ! -f "$VERSION_DIR/CMakeLists.txt" ]; then
        echo "  ⚠️  CMakeLists.txt not found"
        FAILED+=("$version (no CMakeLists.txt)")
        continue
    fi
    
    # Clean and build
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    if eval cmake .. $CMAKE_FLAGS > /dev/null 2>&1 && \
       make ${version}_benchmark -j$CPU_COUNT > /dev/null 2>&1; then
        echo "  ✅ $version built successfully"
        SUCCESSFUL+=("$version")
    else
        echo "  ❌ $version build failed"
        FAILED+=("$version")
    fi
    
    cd - > /dev/null
    echo ""
done

# Summary
echo "=========================================="
echo "Build Summary"
echo "=========================================="
echo "Successful: ${#SUCCESSFUL[@]}"
for v in "${SUCCESSFUL[@]}"; do
    echo "  ✅ $v"
done

echo ""
echo "Failed: ${#FAILED[@]}"
for v in "${FAILED[@]}"; do
    echo "  ❌ $v"
done

echo ""
if [ ${#FAILED[@]} -eq 0 ]; then
    echo "✅ All versions built successfully!"
    exit 0
else
    echo "⚠️  Some versions failed to build"
    exit 1
fi



