cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags for nanosecond latency
# Use generic optimization flags that work on all platforms
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -funroll-loops")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files (exclude main.cpp for library)
file(GLOB_RECURSE CORE_SOURCES
    "src/core/*.cpp"
)

# Remove event_sourcing_advanced.cpp (DistributedEventStore disabled for local testing)
list(REMOVE_ITEM CORE_SOURCES ${CMAKE_SOURCE_DIR}/src/core/event_sourcing_advanced.cpp)

# Remove production-related files that are not needed for benchmark
# These have compilation errors but benchmark doesn't use them
list(REMOVE_ITEM CORE_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_production.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_production_v2.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_production_v3.cpp
)

# Remove ART-related files for now (benchmark only uses basic MatchingEngine)
# These have duplicate definition errors
list(REMOVE_ITEM CORE_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/core/orderbook_art.cpp
    ${CMAKE_SOURCE_DIR}/src/core/orderbook_art_simd.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_art.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_art_simd.cpp
)

# Remove persistence files for benchmark (not needed for basic matching engine)
list(REMOVE_ITEM CORE_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/core/persistence.cpp
    ${CMAKE_SOURCE_DIR}/src/core/persistence_async.cpp
    ${CMAKE_SOURCE_DIR}/src/core/persistence_optimized.cpp
)

# Remove API/server files for benchmark (not needed for basic matching engine)
list(REMOVE_ITEM CORE_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/core/rest_api_server.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api_gateway.cpp
    ${CMAKE_SOURCE_DIR}/src/core/auth_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/notification_service.cpp
    ${CMAKE_SOURCE_DIR}/src/core/monitoring_system.cpp
)

# Remove position.cpp for benchmark (not used by basic matching engine)
list(REMOVE_ITEM CORE_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/core/position.cpp
)

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp ${CORE_SOURCES})

# Benchmark executable
add_executable(benchmark src/benchmark.cpp ${CORE_SOURCES})

# Simple benchmark executable
add_executable(simple_benchmark src/simple_benchmark.cpp ${CORE_SOURCES})

# Quick benchmark executable
add_executable(quick_benchmark src/quick_benchmark.cpp ${CORE_SOURCES})

# Full benchmark executable
add_executable(full_benchmark src/full_benchmark.cpp ${CORE_SOURCES})

# Optimized benchmark executable
add_executable(benchmark_optimized src/benchmark_optimized.cpp ${CORE_SOURCES})

# Quick comparison executable
add_executable(quick_comparison src/quick_comparison.cpp ${CORE_SOURCES})

# SIMD benchmark executable
add_executable(simd_benchmark src/simd_benchmark.cpp ${CORE_SOURCES})

# Comprehensive comparison executable
add_executable(comprehensive_comparison src/comprehensive_comparison.cpp ${CORE_SOURCES})

# Production server executable
add_executable(production_server src/production_main.cpp ${CORE_SOURCES}
    src/core/matching_engine_production.cpp
    src/core/persistence.cpp
    src/core/persistence_optimized.cpp
    src/core/logger.cpp
)

# Persistence benchmark executable
add_executable(persistence_benchmark src/persistence_benchmark.cpp ${CORE_SOURCES})

# Add new source files
list(APPEND CORE_SOURCES
    src/core/order_validator.cpp
    src/core/account_manager.cpp
    src/core/position_manager.cpp
    src/core/matching_engine_optimized_v2.cpp
    src/core/orderbook_optimized.cpp
    src/core/art_tree.cpp
    src/core/orderbook_art.cpp
    src/core/matching_engine_art.cpp
    src/core/art_tree_simd.cpp
    src/core/art_tree_simd_enhanced.cpp
    src/core/orderbook_art_simd.cpp
    src/core/matching_engine_art_simd.cpp
    src/core/event_sourcing.cpp
    src/core/deterministic_calculator.cpp
    src/core/matching_engine_event_sourcing.cpp
    # src/core/event_sourcing_advanced.cpp  # Disabled: DistributedEventStore not needed for local testing
)

# Performance benchmark V2
add_executable(performance_benchmark_v2 src/performance_benchmark_v2.cpp ${CORE_SOURCES})

# Comprehensive performance comparison
add_executable(comprehensive_performance_comparison src/comprehensive_performance_comparison.cpp ${CORE_SOURCES})

# Production V2 benchmark  
add_executable(production_v2_benchmark src/production_v2_benchmark.cpp ${CORE_SOURCES})

# Production V3 benchmark (WAL)
add_executable(production_v3_benchmark src/production_v3_benchmark.cpp src/core/wal_simple.cpp src/core/matching_engine_production_v3.cpp ${CORE_SOURCES})

# Final comparison (all versions)
add_executable(final_comparison src/final_comparison.cpp ${CORE_SOURCES})

# Event Sourcing benchmark
add_executable(event_sourcing_benchmark src/event_sourcing_benchmark.cpp ${CORE_SOURCES})

# Production service
add_executable(production_service src/production_service_main.cpp ${CORE_SOURCES})
target_link_libraries(production_service
    PRIVATE
    pthread
    ssl
    crypto
)

# Basic usage example
add_executable(basic_usage_example examples/basic_usage_example.cpp ${CORE_SOURCES})
target_link_libraries(basic_usage_example
    PRIVATE
    pthread
    ssl
    crypto
)

# Production tests
add_executable(production_load_test tests/production/load_test.cpp ${CORE_SOURCES})
target_link_libraries(production_load_test
    PRIVATE
    pthread
    ssl
    crypto
)

add_executable(production_stress_test tests/production/stress_test.cpp ${CORE_SOURCES})
target_link_libraries(production_stress_test
    PRIVATE
    pthread
    ssl
    crypto
)

add_executable(production_functional_test tests/production/functional_test.cpp ${CORE_SOURCES})
target_link_libraries(production_functional_test
    PRIVATE
    pthread
    ssl
    crypto
)

# Full chain benchmark
add_executable(full_chain_benchmark tests/performance/full_chain_benchmark.cpp ${CORE_SOURCES})
target_link_libraries(full_chain_benchmark
    PRIVATE
    pthread
    ssl
    crypto
)

# Bottleneck analyzer
add_executable(bottleneck_analyzer tests/performance/bottleneck_analyzer.cpp ${CORE_SOURCES})
target_link_libraries(bottleneck_analyzer
    PRIVATE
    pthread
    ssl
    crypto
)

# Add new optimized sources
list(APPEND CORE_SOURCES
    src/core/persistence_async.cpp
    src/core/matching_engine_optimized_v3.cpp
)

# Test optimized V3 (exclude problematic sources)
file(GLOB_RECURSE CORE_SOURCES_FOR_TEST
    "src/core/*.cpp"
)
list(REMOVE_ITEM CORE_SOURCES_FOR_TEST
    "${CMAKE_SOURCE_DIR}/src/core/market_data_service.cpp"
)

add_executable(test_optimized_v3 tests/performance/test_optimized_v3.cpp ${CORE_SOURCES_FOR_TEST})
target_link_libraries(test_optimized_v3
    PRIVATE
    pthread
    ssl
    crypto
)

# Quick test version
add_executable(test_optimized_v3_quick tests/performance/test_optimized_v3_quick.cpp ${CORE_SOURCES_FOR_TEST})
target_link_libraries(test_optimized_v3_quick
    PRIVATE
    pthread
    ssl
    crypto
)

# Minimal test version
add_executable(test_optimized_v3_minimal tests/performance/test_optimized_v3_minimal.cpp ${CORE_SOURCES_FOR_TEST})
target_link_libraries(test_optimized_v3_minimal
    PRIVATE
    pthread
    ssl
    crypto
)

# Fast test version (no rate limiting)
add_executable(test_optimized_v3_fast tests/performance/test_optimized_v3_fast.cpp ${CORE_SOURCES_FOR_TEST})
target_link_libraries(test_optimized_v3_fast
    PRIVATE
    pthread
    ssl
    crypto
)

# Safe test version (conservative, with progress output)
add_executable(test_optimized_v3_safe tests/performance/test_optimized_v3_safe.cpp ${CORE_SOURCES_FOR_TEST})
target_link_libraries(test_optimized_v3_safe
    PRIVATE
    pthread
    ssl
    crypto
)

# Unit / integration / functional tests (fetch GTest if missing)
# Create a separate source list for tests that includes necessary files
file(GLOB_RECURSE TEST_CORE_SOURCES
    "src/core/*.cpp"
)
# Remove problematic files
list(REMOVE_ITEM TEST_CORE_SOURCES 
    ${CMAKE_SOURCE_DIR}/src/core/event_sourcing_advanced.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_production.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_production_v2.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_production_v3.cpp
    ${CMAKE_SOURCE_DIR}/src/core/orderbook_art.cpp
    ${CMAKE_SOURCE_DIR}/src/core/orderbook_art_simd.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_art.cpp
    ${CMAKE_SOURCE_DIR}/src/core/matching_engine_art_simd.cpp
    ${CMAKE_SOURCE_DIR}/src/core/persistence.cpp
    ${CMAKE_SOURCE_DIR}/src/core/persistence_async.cpp
    ${CMAKE_SOURCE_DIR}/src/core/persistence_optimized.cpp
    ${CMAKE_SOURCE_DIR}/src/core/rest_api_server.cpp
    ${CMAKE_SOURCE_DIR}/src/core/api_gateway.cpp
    ${CMAKE_SOURCE_DIR}/src/core/notification_service.cpp
    ${CMAKE_SOURCE_DIR}/src/core/monitoring_system.cpp
    ${CMAKE_SOURCE_DIR}/src/core/market_data_service.cpp
    ${CMAKE_SOURCE_DIR}/src/core/position.cpp
)

list(REMOVE_ITEM CORE_SOURCES ${CMAKE_SOURCE_DIR}/src/core/market_data_service.cpp)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "Google Test not found, fetching via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

if(GTest_FOUND OR googletest_POPULATED)
    add_executable(test_auth_manager tests/unit/test_auth_manager.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_auth_manager
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_liquidation_engine tests/unit/test_liquidation_engine.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_liquidation_engine
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_funding_rate_manager tests/unit/test_funding_rate_manager.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_funding_rate_manager
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_matching_engine_event_sourcing tests/unit/test_matching_engine_event_sourcing.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_matching_engine_event_sourcing
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_account_manager tests/unit/test_account_manager.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_account_manager
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_position_manager tests/unit/test_position_manager.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_position_manager
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_orderbook tests/unit/test_orderbook.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_orderbook
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_order_validator tests/unit/test_order_validator.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_order_validator
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_trading_workflow tests/integration/test_trading_workflow.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_trading_workflow
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    add_executable(test_production_performance tests/performance/test_production_performance.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_production_performance
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )

    add_executable(test_comprehensive_functional tests/functional/comprehensive_functional_test.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_comprehensive_functional
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )

    add_executable(test_order_flow tests/functional/order_flow_test.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_order_flow
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )

    add_executable(test_api_integration tests/functional/api_integration_test.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_api_integration
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )

    add_executable(test_consensus tests/functional/consensus_test.cpp ${TEST_CORE_SOURCES})
    target_link_libraries(test_consensus
        PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
        ssl
        crypto
    )
    
    enable_testing()
    add_test(NAME AuthManagerTest COMMAND test_auth_manager)
    add_test(NAME LiquidationEngineTest COMMAND test_liquidation_engine)
    add_test(NAME FundingRateManagerTest COMMAND test_funding_rate_manager)
    add_test(NAME MatchingEngineEventSourcingTest COMMAND test_matching_engine_event_sourcing)
    add_test(NAME AccountManagerTest COMMAND test_account_manager)
    add_test(NAME PositionManagerTest COMMAND test_position_manager)
    add_test(NAME OrderBookTest COMMAND test_orderbook)
    add_test(NAME OrderValidatorTest COMMAND test_order_validator)
    add_test(NAME TradingWorkflowTest COMMAND test_trading_workflow)
    add_test(NAME ProductionPerformanceTest COMMAND test_production_performance)
    add_test(NAME ComprehensiveFunctionalTest COMMAND test_comprehensive_functional)
    add_test(NAME OrderFlowTest COMMAND test_order_flow)
    add_test(NAME APIIntegrationTest COMMAND test_api_integration)
    add_test(NAME ConsensusTest COMMAND test_consensus)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    pthread
    ssl
    crypto
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Microservices
# Check if gRPC is available
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GRPC QUIET grpc++)
endif()

# Matching Service
if(GRPC_FOUND OR DEFINED ENV{GRPC_INSTALL_DIR})
    add_executable(matching_service
        services/matching_service/main.cpp
        services/matching_service/matching_service.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(matching_service
        PRIVATE
        pthread
        grpc++
        protobuf
    )
    target_include_directories(matching_service PRIVATE
        ${CMAKE_BINARY_DIR}  # For generated proto files
    )
endif()

# Trading Service
if(GRPC_FOUND OR DEFINED ENV{GRPC_INSTALL_DIR})
    add_executable(trading_service
        services/trading_service/main.cpp
        services/trading_service/trading_service.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(trading_service
        PRIVATE
        pthread
        grpc++
        protobuf
    )
    target_include_directories(trading_service PRIVATE
        ${CMAKE_BINARY_DIR}  # For generated proto files
    )
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GRPC QUIET grpc++)
endif()

# Matching Service
if(GRPC_FOUND OR DEFINED ENV{GRPC_INSTALL_DIR})
    add_executable(matching_service
        services/matching_service/main.cpp
        services/matching_service/matching_service.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(matching_service
        PRIVATE
        pthread
        grpc++
        protobuf
    )
    target_include_directories(matching_service PRIVATE
        ${CMAKE_BINARY_DIR}  # For generated proto files
    )
endif()

# Trading Service
if(GRPC_FOUND OR DEFINED ENV{GRPC_INSTALL_DIR})
    add_executable(trading_service
        services/trading_service/main.cpp
        services/trading_service/trading_service.cpp
        ${CORE_SOURCES}
    )
    target_link_libraries(trading_service
        PRIVATE
        pthread
        grpc++
        protobuf
    )
    target_include_directories(trading_service PRIVATE
        ${CMAKE_BINARY_DIR}  # For generated proto files
    )
endif()
