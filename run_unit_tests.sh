#!/bin/bash

# Unit Tests Runner Script
# 运行所有单元测试并生成报告

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "Unit Tests Runner"
echo "========================================"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Build directory not found. Creating build directory...${NC}"
    mkdir -p build
    cd build
    cmake ..
    cd ..
fi

# Build all unit tests
echo -e "${YELLOW}Building unit tests...${NC}"
cd build

# Build tests
cmake --build . --target test_account_manager test_position_manager test_orderbook test_order_validator \
    test_auth_manager test_liquidation_engine test_funding_rate_manager test_matching_engine_event_sourcing 2>&1 | tee build_tests.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Build successful!${NC}"
echo ""

# Create results directory
mkdir -p test_results
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_DIR="test_results/unit_tests_${TIMESTAMP}"
mkdir -p "${RESULTS_DIR}"

# Run all unit tests
echo "========================================"
echo "Running Unit Tests"
echo "========================================"
echo ""

# List of unit tests
TESTS=(
    "test_account_manager:AccountManager"
    "test_position_manager:PositionManager"
    "test_orderbook:OrderBook"
    "test_order_validator:OrderValidator"
    "test_auth_manager:AuthManager"
    "test_liquidation_engine:LiquidationEngine"
    "test_funding_rate_manager:FundingRateManager"
    "test_matching_engine_event_sourcing:MatchingEngineEventSourcing"
)

PASSED=0
FAILED=0
TOTAL=${#TESTS[@]}

for test_info in "${TESTS[@]}"; do
    IFS=':' read -r test_executable test_name <<< "$test_info"
    
    echo -e "${YELLOW}Running ${test_name} tests...${NC}"
    
    if [ -f "./${test_executable}" ]; then
        # Run test and capture output
        if ./${test_executable} --gtest_output=xml:${RESULTS_DIR}/${test_executable}.xml > ${RESULTS_DIR}/${test_executable}.log 2>&1; then
            echo -e "${GREEN}✅ ${test_name} - PASSED${NC}"
            ((PASSED++))
        else
            echo -e "${RED}❌ ${test_name} - FAILED${NC}"
            ((FAILED++))
            # Show last few lines of error
            echo "Last 10 lines of output:"
            tail -n 10 ${RESULTS_DIR}/${test_executable}.log
        fi
    else
        echo -e "${RED}❌ ${test_executable} not found!${NC}"
        ((FAILED++))
    fi
    echo ""
done

# Summary
echo "========================================"
echo "Test Summary"
echo "========================================"
echo -e "Total Tests: ${TOTAL}"
echo -e "${GREEN}Passed: ${PASSED}${NC}"
if [ ${FAILED} -gt 0 ]; then
    echo -e "${RED}Failed: ${FAILED}${NC}"
else
    echo -e "${GREEN}Failed: ${FAILED}${NC}"
fi
echo ""
echo "Results saved to: ${RESULTS_DIR}/"
echo ""

# Generate summary report
SUMMARY_FILE="${RESULTS_DIR}/summary.txt"
cat > "${SUMMARY_FILE}" << EOF
Unit Tests Summary
==================
Date: $(date)
Total Tests: ${TOTAL}
Passed: ${PASSED}
Failed: ${FAILED}

Test Results:
EOF

for test_info in "${TESTS[@]}"; do
    IFS=':' read -r test_executable test_name <<< "$test_info"
    if [ -f "./${test_executable}" ]; then
        if ./${test_executable} --gtest_brief=1 > /dev/null 2>&1; then
            echo "✅ ${test_name}" >> "${SUMMARY_FILE}"
        else
            echo "❌ ${test_name}" >> "${SUMMARY_FILE}"
        fi
    else
        echo "❌ ${test_name} (not found)" >> "${SUMMARY_FILE}"
    fi
done

echo "Summary report: ${SUMMARY_FILE}"
echo ""

# Exit with error if any tests failed
if [ ${FAILED} -gt 0 ]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi

