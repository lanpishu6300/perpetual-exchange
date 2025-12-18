cmake_minimum_required(VERSION 3.15)
project(PerpetualExchange_EventSourcing VERSION 1.0.0 LANGUAGES CXX)

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

# Source files (Event Sourcing depends on Original)
set(EVENT_SOURCING_SOURCES
    ${PROJECT_ROOT}/versions/original/src/matching_engine.cpp
    ${PROJECT_ROOT}/versions/original/src/orderbook.cpp
    ${CMAKE_SOURCE_DIR}/src/matching_engine_event_sourcing.cpp
)

# Add local event_sourcing source files if they exist
if(EXISTS "${CMAKE_SOURCE_DIR}/src/core/event_sourcing.cpp")
    list(APPEND EVENT_SOURCING_SOURCES "${CMAKE_SOURCE_DIR}/src/core/event_sourcing.cpp")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/src/core/deterministic_calculator.cpp")
    list(APPEND EVENT_SOURCING_SOURCES "${CMAKE_SOURCE_DIR}/src/core/deterministic_calculator.cpp")
endif()

# Event sourcing dependencies - use local copies if available
file(GLOB_RECURSE ES_DEPS 
    "${PROJECT_ROOT}/src/core/event_sourcing.cpp"
    "${PROJECT_ROOT}/src/core/deterministic_calculator.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/event_sourcing.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/deterministic_calculator.cpp"
)

foreach(dep ${ES_DEPS})
    if(EXISTS ${dep})
        list(APPEND EVENT_SOURCING_SOURCES ${dep})
    endif()
endforeach()

# If no event_sourcing.cpp found, create a minimal stub
if(NOT EVENT_SOURCING_SOURCES MATCHES "event_sourcing.cpp")
    message(WARNING "event_sourcing.cpp not found, event sourcing features may be limited")
endif()

# Create library
add_library(perpetual_event_sourcing STATIC ${EVENT_SOURCING_SOURCES})

# Benchmark executable
add_executable(event_sourcing_benchmark benchmark.cpp)
target_link_libraries(event_sourcing_benchmark perpetual_event_sourcing)
