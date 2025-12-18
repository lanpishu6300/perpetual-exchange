cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_ARTSIMD VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags with SIMD support
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -funroll-loops -mavx2 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Get project root
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${PROJECT_ROOT}/include)
include_directories(${PROJECT_ROOT}/versions/common/include)
include_directories(${PROJECT_ROOT}/versions/original/include)
include_directories(${PROJECT_ROOT}/versions/art/include)

# Source files
set(ART_SIMD_SOURCES
    ${PROJECT_ROOT}/versions/original/src/matching_engine.cpp
    ${PROJECT_ROOT}/versions/original/src/orderbook.cpp
    ${PROJECT_ROOT}/versions/art/src/art_tree.cpp
    ${PROJECT_ROOT}/versions/art/src/orderbook_art.cpp
    ${PROJECT_ROOT}/versions/art/src/matching_engine_art.cpp
)

# Use local source files if available, otherwise fall back to project root
if(EXISTS "${CMAKE_SOURCE_DIR}/src/art_tree_simd.cpp")
    list(APPEND ART_SIMD_SOURCES "${CMAKE_SOURCE_DIR}/src/art_tree_simd.cpp")
elseif(EXISTS "${PROJECT_ROOT}/src/core/art_tree_simd.cpp")
    list(APPEND ART_SIMD_SOURCES "${PROJECT_ROOT}/src/core/art_tree_simd.cpp")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/src/art_tree_simd_enhanced.cpp")
    list(APPEND ART_SIMD_SOURCES "${CMAKE_SOURCE_DIR}/src/art_tree_simd_enhanced.cpp")
elseif(EXISTS "${PROJECT_ROOT}/src/core/art_tree_simd_enhanced.cpp")
    list(APPEND ART_SIMD_SOURCES "${PROJECT_ROOT}/src/core/art_tree_simd_enhanced.cpp")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/src/orderbook_art_simd.cpp")
    list(APPEND ART_SIMD_SOURCES "${CMAKE_SOURCE_DIR}/src/orderbook_art_simd.cpp")
elseif(EXISTS "${PROJECT_ROOT}/src/core/orderbook_art_simd.cpp")
    list(APPEND ART_SIMD_SOURCES "${PROJECT_ROOT}/src/core/orderbook_art_simd.cpp")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/src/matching_engine_art_simd.cpp")
    list(APPEND ART_SIMD_SOURCES "${CMAKE_SOURCE_DIR}/src/matching_engine_art_simd.cpp")
elseif(EXISTS "${PROJECT_ROOT}/src/core/matching_engine_art_simd.cpp")
    list(APPEND ART_SIMD_SOURCES "${PROJECT_ROOT}/src/core/matching_engine_art_simd.cpp")
endif()

# Create library
add_library(perpetual_art_simd STATIC ${ART_SIMD_SOURCES})

# Benchmark executable
add_executable(art_simd_benchmark benchmark.cpp)
target_link_libraries(art_simd_benchmark perpetual_art_simd)
