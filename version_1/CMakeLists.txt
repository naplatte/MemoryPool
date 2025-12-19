# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name
project(MemoryPoolV1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files (only version_1)
set(SOURCES
    src/MemoryPool.cpp
)

# Header files (only version_1)
set(HEADERS
    include/MemoryPool.h
)

# Test files (only version_1)
set(TEST_FILES
    tests/UnitTest.cpp
)

# Add the executable for the test target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${TEST_FILES})

# Add compile options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -g)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()
