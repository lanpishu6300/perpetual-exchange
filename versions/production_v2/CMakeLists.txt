cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_ProductionV2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -funroll-loops -mavx2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Include directories - only use local includes, no dependencies on other versions
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/core)

# Source files - all from local directory, completely independent
file(GLOB_RECURSE PRODUCTION_V2_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Exclude files that are not needed or cause compilation errors
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*event_sourcing.*")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*deterministic_calculator.*")

# Create library - completely self-contained
add_library(perpetual_production_v2 STATIC ${PRODUCTION_V2_SOURCES})
