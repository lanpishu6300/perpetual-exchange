cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_ProductionFast VERSION 1.0.0 LANGUAGES CXX)

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
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*persistence_async.*")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*position\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*rest_api_server\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*api_gateway\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*auth_manager\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*notification_service\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*monitoring_system\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*database_manager\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*market_data_service\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*wal_simple\\.cpp$")
list(FILTER PRODUCTION_V2_SOURCES EXCLUDE REGEX ".*orderbook_art_simd.*")

# Add orderbook_art_simd if needed (for ART+SIMD support) - must be before add_library
if(EXISTS "${CMAKE_SOURCE_DIR}/src/core/orderbook_art_simd.cpp")
    list(APPEND PRODUCTION_V2_SOURCES "${CMAKE_SOURCE_DIR}/src/core/orderbook_art_simd.cpp")
endif()

# Create library - completely self-contained
# Production Fast: High-performance version with ART+SIMD optimization (~450K orders/sec)
add_library(perpetual_production_fast STATIC ${PRODUCTION_V2_SOURCES})

# Benchmark executable
add_executable(production_fast_benchmark benchmark.cpp)
target_link_libraries(production_fast_benchmark perpetual_production_fast)
