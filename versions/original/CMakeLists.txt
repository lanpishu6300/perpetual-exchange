cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_Original VERSION 1.0.0 LANGUAGES CXX)

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

# Source files
set(ORIGINAL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/matching_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/orderbook.cpp
)

# Create library
add_library(perpetual_original STATIC ${ORIGINAL_SOURCES})

# Benchmark executable
add_executable(original_benchmark benchmark.cpp)
target_link_libraries(original_benchmark perpetual_original)
