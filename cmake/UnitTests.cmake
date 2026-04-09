enable_testing()
file(GLOB_RECURSE TEST_SOURCES "src/UnitTests/*.cpp")

if(TEST_SOURCES AND NOT EMSCRIPTEN)
    add_executable(unit_tests ${TEST_SOURCES})
    target_link_libraries(unit_tests PRIVATE 65c02_core_test GTest::gtest_main)

    if(WIN32)
        target_link_options(unit_tests PRIVATE -static -static-libgcc -static-libstdc++)
    else()
        target_link_options(unit_tests PRIVATE -static-libgcc -static-libstdc++)
    endif()

    include(GoogleTest)
    if(NOT CMAKE_CROSSCOMPILING)
        gtest_discover_tests(unit_tests)
    endif()
endif()
