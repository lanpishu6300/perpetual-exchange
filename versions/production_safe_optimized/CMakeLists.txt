cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_ProductionSafeOptimized)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Get project root
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_ROOT}/include
    ${PROJECT_ROOT}/versions/production_safe/include
    ${PROJECT_ROOT}/versions/production_fast/include
)

# Source files
set(SAFE_OPTIMIZED_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/matching_engine_production_safe_optimized.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sharded_matching_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/trading_shard.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/matching_engine_production_v2.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/matching_engine_production.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/wal_simple.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/matching_engine_art_simd.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/matching_engine_art.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/matching_engine.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/config.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/health_check.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/persistence.cpp
    ${PROJECT_ROOT}/src/core/logger.cpp
    ${PROJECT_ROOT}/src/core/metrics.cpp
    ${PROJECT_ROOT}/src/core/order_validator.cpp
    ${PROJECT_ROOT}/src/core/rate_limiter.cpp
    ${PROJECT_ROOT}/src/core/account.cpp
    ${PROJECT_ROOT}/src/core/account_manager.cpp
    ${PROJECT_ROOT}/src/core/position_manager.cpp
    ${PROJECT_ROOT}/src/core/liquidation_engine.cpp
    ${PROJECT_ROOT}/versions/original/src/orderbook.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/orderbook_art.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/art_tree.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/orderbook_art_simd.cpp
    ${PROJECT_ROOT}/versions/production_safe/src/core/art_tree_simd.cpp
)

# Create library
add_library(perpetual_production_safe_optimized STATIC ${SAFE_OPTIMIZED_SOURCES})

# Compiler flags - avoid -march=native on Apple Silicon
# Force remove -march=native for Apple Silicon compatibility
if(APPLE)
    # Apple platforms - never use -march=native (causes issues on Apple Silicon)
    target_compile_options(perpetual_production_safe_optimized PRIVATE
        -O3
        -flto
        -funroll-loops
        -Wall
        -Wextra
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    # x86_64 Linux - can use -march=native
    target_compile_options(perpetual_production_safe_optimized PRIVATE
        -O3
        -flto
        -funroll-loops
        -march=native
        -Wall
        -Wextra
    )
else()
    # Other architectures
    target_compile_options(perpetual_production_safe_optimized PRIVATE
        -O3
        -flto
        -funroll-loops
        -Wall
        -Wextra
    )
endif()

# Benchmark executable
add_executable(production_safe_optimized_benchmark
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp
)

target_link_libraries(production_safe_optimized_benchmark
    perpetual_production_safe_optimized
    pthread
)

# Zero loss benchmark executable
add_executable(production_safe_optimized_benchmark_zero_loss
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_zero_loss.cpp
)

target_link_libraries(production_safe_optimized_benchmark_zero_loss
    perpetual_production_safe_optimized
    pthread
)

# Realtime benchmark executable
add_executable(production_safe_optimized_realtime_benchmark
    ${CMAKE_CURRENT_SOURCE_DIR}/realtime_benchmark.cpp
)

target_link_libraries(production_safe_optimized_realtime_benchmark
    perpetual_production_safe_optimized
    pthread
)

# Sharded benchmark executable
add_executable(production_safe_optimized_sharded_benchmark
    ${CMAKE_CURRENT_SOURCE_DIR}/sharded_benchmark.cpp
)

target_link_libraries(production_safe_optimized_sharded_benchmark
    perpetual_production_safe_optimized
    pthread
)

# Sharded engine unit test
add_executable(production_safe_optimized_sharded_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sharded_engine.cpp
)

target_link_libraries(production_safe_optimized_sharded_test
    perpetual_production_safe_optimized
    pthread
)

# Quick stress test
add_executable(production_safe_optimized_quick_stress_test
    ${CMAKE_CURRENT_SOURCE_DIR}/quick_stress_test.cpp
)

target_link_libraries(production_safe_optimized_quick_stress_test
    perpetual_production_safe_optimized
    pthread
)

# Trading shard benchmark (独立测试交易分片)
add_executable(production_safe_optimized_benchmark_trading_shard
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_trading_shard.cpp
)

target_link_libraries(production_safe_optimized_benchmark_trading_shard
    perpetual_production_safe_optimized
    pthread
)

# Matching shard benchmark (独立测试撮合分片)
add_executable(production_safe_optimized_benchmark_matching_shard
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_matching_shard.cpp
)

target_link_libraries(production_safe_optimized_benchmark_matching_shard
    perpetual_production_safe_optimized
    pthread
)

# Full shard benchmark executable (Trading + Matching)
add_executable(production_safe_optimized_full_shard_benchmark
    ${CMAKE_CURRENT_SOURCE_DIR}/full_shard_benchmark.cpp
)

target_link_libraries(production_safe_optimized_full_shard_benchmark
    perpetual_production_safe_optimized
    pthread
)

# Force remove -march=native for Apple platforms
if(APPLE)
    # Apple platforms - never use -march=native
    target_compile_options(production_safe_optimized_benchmark PRIVATE
        -O3
        -flto
        -funroll-loops
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    # x86_64 Linux - can use -march=native
    target_compile_options(production_safe_optimized_benchmark PRIVATE
        -O3
        -flto
        -funroll-loops
        -march=native
    )
else()
    # Other architectures
    target_compile_options(production_safe_optimized_benchmark PRIVATE
        -O3
        -flto
        -funroll-loops
    )
endif()
