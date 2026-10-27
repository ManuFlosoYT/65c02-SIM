enable_testing()
file(GLOB_RECURSE TEST_SOURCES "src/UnitTests/*.cpp")

if(TEST_SOURCES AND NOT EMSCRIPTEN)
    add_executable(unit_tests ${TEST_SOURCES} "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDIFile.cpp" "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDISoundBank.cpp" "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDISoundBankLoader.cpp" "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDIVoiceAllocator.cpp")
    set_target_properties(unit_tests PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_BATCH_SIZE 16
        UNITY_BUILD_MODE BATCH
    )
    set_source_files_properties(
        "src/UnitTests/System_Savestate_Test.cpp"
        "src/UnitTests/System_WAI_Threading_Test.cpp"
        "src/UnitTests/System_Cartridge_Test.cpp"
        "src/UnitTests/RasterBeam_Test.cpp"
        "src/UnitTests/LogicAnalyzer_Test.cpp"
        "src/UnitTests/Interrupts_Test.cpp"
        "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDIFile.cpp"
        "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDISoundBank.cpp"
        "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDISoundBankLoader.cpp"
        "${CMAKE_SOURCE_DIR}/src/Frontend/GUI/MIDI/MIDIVoiceAllocator.cpp"
        PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
    )
    target_precompile_headers(unit_tests PRIVATE
        <gtest/gtest.h>
        <cstdint>
        <vector>
        <string>
        <memory>
        <algorithm>
        "${CMAKE_SOURCE_DIR}/src/Hardware/CPU/CPU.h"
        "${CMAKE_SOURCE_DIR}/src/Hardware/Core/Bus.h"
        "${CMAKE_SOURCE_DIR}/src/Hardware/Memory/RAM.h"
        "${CMAKE_SOURCE_DIR}/src/Hardware/CPU/Instructions/InstructionSet.h"
    )
    target_link_libraries(unit_tests PRIVATE 65c02_core_test GTest::gtest_main)

    if(WIN32)
        target_link_options(unit_tests PRIVATE -static -static-libgcc -static-libstdc++)
    else()
        target_link_options(unit_tests PRIVATE -static-libgcc -static-libstdc++)
    endif()

    include(GoogleTest)
    if(NOT CMAKE_CROSSCOMPILING)
        gtest_discover_tests(unit_tests
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            PROPERTIES TIMEOUT 30
        )
    endif()
endif()
