cmake_minimum_required(VERSION 3.31)
project(SIM_65C02)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB CORE_SOURCES
    "Instrucciones/*.cpp"
    "Instrucciones/*.h"
    "Componentes/*.cpp"
    "Componentes/*.h"
)

add_library(65c02_core STATIC ${CORE_SOURCES})
add_library(65c02_core_test STATIC ${CORE_SOURCES})
target_compile_definitions(65c02_core_test PUBLIC TESTING_ENV)

add_executable(SIM_65C02 main.cpp)
target_link_libraries(SIM_65C02 PRIVATE 65c02_core)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
file(GLOB_RECURSE TEST_SOURCES "UnitTests/*.cpp")

if(TEST_SOURCES)
    add_executable(unit_tests ${TEST_SOURCES})
    target_link_libraries(unit_tests PRIVATE 65c02_core_test GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()
