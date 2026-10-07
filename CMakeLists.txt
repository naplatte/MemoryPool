# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# Project name
project(MemoryPool)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/ThreadCache.cpp
    src/CentralCache.cpp
    src/PageCache.cpp
)

# Test files
set(TEST_FILES
    tests/UnitTest.cpp
)

# Add the executable for unit tests
add_executable(${PROJECT_NAME}_UnitTest ${SOURCES} ${TEST_FILES})

# Add the executable for performance tests
add_executable(${PROJECT_NAME}_PerformanceTest ${SOURCES} tests/PerformanceTest.cpp)

# Enable testing
enable_testing()
add_test(NAME ${PROJECT_NAME}_UnitTest COMMAND ${PROJECT_NAME}_UnitTest)
add_test(NAME ${PROJECT_NAME}_PerformanceTest COMMAND ${PROJECT_NAME}_PerformanceTest)

# Add compile options
if(MSVC)
    target_compile_options(${PROJECT_NAME}_UnitTest PRIVATE /W4 /O2 /UNDEBUG)
    target_compile_options(${PROJECT_NAME}_PerformanceTest PRIVATE /W4 /O2)
else()
    target_compile_options(${PROJECT_NAME}_UnitTest PRIVATE -Wall -Wextra -O2 -g -UNDEBUG)
    target_compile_options(${PROJECT_NAME}_PerformanceTest PRIVATE -Wall -Wextra -O2 -g)
    target_link_libraries(${PROJECT_NAME}_UnitTest pthread)
    target_link_libraries(${PROJECT_NAME}_PerformanceTest pthread)
    if(MINGW)
        # Avoid loading an incompatible MinGW runtime DLL from PATH.
        target_link_options(${PROJECT_NAME}_UnitTest PRIVATE -static)
        target_link_options(${PROJECT_NAME}_PerformanceTest PRIVATE -static)
    endif()
endif()


# Keep CTest logs readable on Windows, including localized timestamps.
if(WIN32)
    configure_file(cmake/CTestCustom.cmake CTestCustom.cmake COPYONLY)
endif()
set_tests_properties(${PROJECT_NAME}_PerformanceTest PROPERTIES TIMEOUT 240)

# Keep Chinese test messages UTF-8 in both terminal output and log files.
foreach(test_target ${PROJECT_NAME}_UnitTest ${PROJECT_NAME}_PerformanceTest)
    if(MSVC)
        target_compile_options(${test_target} PRIVATE /utf-8)
    else()
        target_compile_options(${test_target} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    endif()
endforeach()
