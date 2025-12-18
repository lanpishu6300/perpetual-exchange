cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_ART VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Get project root
get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${PROJECT_ROOT}/include)
include_directories(${PROJECT_ROOT}/versions/common/include)
include_directories(${PROJECT_ROOT}/versions/original/include)

# Source files
set(ART_SOURCES
    ${PROJECT_ROOT}/versions/original/src/matching_engine.cpp
    ${PROJECT_ROOT}/versions/original/src/orderbook.cpp
    ${CMAKE_SOURCE_DIR}/src/art_tree.cpp
    ${CMAKE_SOURCE_DIR}/src/orderbook_art.cpp
    ${CMAKE_SOURCE_DIR}/src/matching_engine_art.cpp
)

# Create library
add_library(perpetual_art STATIC ${ART_SOURCES})

# Benchmark executable
add_executable(art_benchmark benchmark.cpp)
target_link_libraries(art_benchmark perpetual_art)
