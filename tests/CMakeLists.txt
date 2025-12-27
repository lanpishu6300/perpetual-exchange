# Testing configuration
cmake_minimum_required(VERSION 3.15)

# Find Google Test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(WARNING "Google Test not found. Building without tests.")
    return()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Core sources (excluding main.cpp)
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
)

# Unit tests
add_executable(test_auth_manager
    tests/unit/test_auth_manager.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_auth_manager
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_liquidation_engine
    tests/unit/test_liquidation_engine.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_liquidation_engine
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_funding_rate_manager
    tests/unit/test_funding_rate_manager.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_funding_rate_manager
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_matching_engine_event_sourcing
    tests/unit/test_matching_engine_event_sourcing.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_matching_engine_event_sourcing
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_account_manager
    tests/unit/test_account_manager.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_account_manager
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_position_manager
    tests/unit/test_position_manager.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_position_manager
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_orderbook
    tests/unit/test_orderbook.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_orderbook
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_order_validator
    tests/unit/test_order_validator.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_order_validator
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

# Integration tests
add_executable(test_trading_workflow
    tests/integration/test_trading_workflow.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_trading_workflow
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

# Performance tests
add_executable(test_production_performance
    tests/performance/test_production_performance.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_production_performance
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

# Functional tests
add_executable(test_comprehensive_functional
    tests/functional/comprehensive_functional_test.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_comprehensive_functional
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_order_flow
    tests/functional/order_flow_test.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_order_flow
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_api_integration
    tests/functional/api_integration_test.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_api_integration
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

add_executable(test_consensus
    tests/functional/consensus_test.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_consensus
    PRIVATE
    GTest::GTest
    GTest::Main
    pthread
    ssl
    crypto
)

# Enable testing
enable_testing()

# Add tests
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
