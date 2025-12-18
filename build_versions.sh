#!/bin/bash

# 构建所有版本脚本

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_ROOT"

echo "=========================================="
echo "构建所有版本"
echo "=========================================="
echo ""

VERSIONS=("original" "optimized" "optimized_v2" "art" "art_simd" "production_basic" "production_fast" "production_safe" "event_sourcing")

BUILD_SUCCESS=()
BUILD_FAILED=()

for version in "${VERSIONS[@]}"; do
    echo "----------------------------------------"
    echo "构建版本: $version"
    echo "----------------------------------------"
    
    VERSION_DIR="versions/$version"
    BUILD_DIR="$VERSION_DIR/build"
    
    if [ ! -d "$VERSION_DIR" ]; then
        echo "⚠️  版本目录不存在: $VERSION_DIR"
        BUILD_FAILED+=("$version (目录不存在)")
        continue
    fi
    
    if [ ! -f "$VERSION_DIR/CMakeLists.txt" ]; then
        echo "⚠️  CMakeLists.txt不存在，跳过"
        BUILD_FAILED+=("$version (CMakeLists.txt不存在)")
        continue
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    if cmake .. > /dev/null 2>&1; then
        if make > /dev/null 2>&1; then
            echo "✅ $version 构建成功"
            BUILD_SUCCESS+=("$version")
        else
            echo "❌ $version 编译失败"
            BUILD_FAILED+=("$version (编译错误)")
        fi
    else
        echo "❌ $version CMake配置失败"
        BUILD_FAILED+=("$version (CMake错误)")
    fi
    
    cd "$PROJECT_ROOT"
    echo ""
done

echo "=========================================="
echo "构建总结"
echo "=========================================="
echo "✅ 成功: ${#BUILD_SUCCESS[@]} 个版本"
for v in "${BUILD_SUCCESS[@]}"; do
    echo "   - $v"
done

echo ""
echo "❌ 失败: ${#BUILD_FAILED[@]} 个版本"
for v in "${BUILD_FAILED[@]}"; do
    echo "   - $v"
done


# 构建所有版本脚本

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_ROOT"

echo "=========================================="
echo "构建所有版本"
echo "=========================================="
echo ""

VERSIONS=("original" "optimized" "optimized_v2" "art" "art_simd" "production_basic" "production_fast" "production_safe" "event_sourcing")

BUILD_SUCCESS=()
BUILD_FAILED=()

for version in "${VERSIONS[@]}"; do
    echo "----------------------------------------"
    echo "构建版本: $version"
    echo "----------------------------------------"
    
    VERSION_DIR="versions/$version"
    BUILD_DIR="$VERSION_DIR/build"
    
    if [ ! -d "$VERSION_DIR" ]; then
        echo "⚠️  版本目录不存在: $VERSION_DIR"
        BUILD_FAILED+=("$version (目录不存在)")
        continue
    fi
    
    if [ ! -f "$VERSION_DIR/CMakeLists.txt" ]; then
        echo "⚠️  CMakeLists.txt不存在，跳过"
        BUILD_FAILED+=("$version (CMakeLists.txt不存在)")
        continue
    fi
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    if cmake .. > /dev/null 2>&1; then
        if make > /dev/null 2>&1; then
            echo "✅ $version 构建成功"
            BUILD_SUCCESS+=("$version")
        else
            echo "❌ $version 编译失败"
            BUILD_FAILED+=("$version (编译错误)")
        fi
    else
        echo "❌ $version CMake配置失败"
        BUILD_FAILED+=("$version (CMake错误)")
    fi
    
    cd "$PROJECT_ROOT"
    echo ""
done

echo "=========================================="
echo "构建总结"
echo "=========================================="
echo "✅ 成功: ${#BUILD_SUCCESS[@]} 个版本"
for v in "${BUILD_SUCCESS[@]}"; do
    echo "   - $v"
done

echo ""
echo "❌ 失败: ${#BUILD_FAILED[@]} 个版本"
for v in "${BUILD_FAILED[@]}"; do
    echo "   - $v"
done

