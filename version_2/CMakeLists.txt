# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name
project(MemoryPoolV2)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories (only version_2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files (only version_2)
set(SOURCES
    src/ThreadCache.cpp
    src/CentralCache.cpp
    src/PageCache.cpp
)

# Header files (only version_2)
set(HEADERS
    include/utils.h
    include/ThreadCache.h
    include/CentralCache.h
    include/PageCache.h
    include/MemoryPool.h
)

# Test files (only version_2)
set(TEST_FILES
    tests/UnitTest.cpp
)

# Add the executable for unit tests
add_executable(${PROJECT_NAME}_UnitTest ${SOURCES} ${HEADERS} ${TEST_FILES})

# Add the executable for performance tests
add_executable(${PROJECT_NAME}_PerformanceTest ${SOURCES} ${HEADERS} tests/PerformanceTest.cpp)

# Add compile options
if(MSVC)
    target_compile_options(${PROJECT_NAME}_UnitTest PRIVATE /W4 /O2)
    target_compile_options(${PROJECT_NAME}_PerformanceTest PRIVATE /W4 /O2)
else()
    target_compile_options(${PROJECT_NAME}_UnitTest PRIVATE -Wall -Wextra -O2 -g)
    target_compile_options(${PROJECT_NAME}_PerformanceTest PRIVATE -Wall -Wextra -O2 -g)
    target_link_libraries(${PROJECT_NAME}_UnitTest pthread)
    target_link_libraries(${PROJECT_NAME}_PerformanceTest pthread)
endif()

