cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_Optimized VERSION 1.0.0 LANGUAGES CXX)

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
set(OPTIMIZED_SOURCES
    ${PROJECT_ROOT}/versions/original/src/matching_engine.cpp
    ${PROJECT_ROOT}/versions/original/src/orderbook.cpp
    ${CMAKE_SOURCE_DIR}/src/matching_engine_optimized.cpp
)

# Dependencies from main project (if they exist)
file(GLOB_RECURSE MEMORY_POOL "${PROJECT_ROOT}/src/core/memory_pool.cpp")
if(MEMORY_POOL)
    list(APPEND OPTIMIZED_SOURCES ${MEMORY_POOL})
endif()

file(GLOB_RECURSE LOCKFREE_QUEUE "${PROJECT_ROOT}/src/core/lockfree_queue.cpp")
if(LOCKFREE_QUEUE)
    list(APPEND OPTIMIZED_SOURCES ${LOCKFREE_QUEUE})
endif()

# Create library
add_library(perpetual_optimized STATIC ${OPTIMIZED_SOURCES})

# Benchmark executable
add_executable(optimized_benchmark benchmark.cpp)
target_link_libraries(optimized_benchmark perpetual_optimized)
