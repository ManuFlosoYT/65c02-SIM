add_executable(SIM_65C02
    src/Hardware/Audio/NSFRunner_main.cpp
    src/Frontend/CliArgsParser.cpp
    src/Hardware/Audio/NSFRunner.cpp
    src/Hardware/CPU/CPU.cpp
    src/Hardware/CPU/Instructions/ControlFlow.cpp
    src/Hardware/Core/Bus.cpp
    src/Hardware/Core/BusTelemetry.cpp
    src/Hardware/Core/SubroutineProfiler.cpp
    src/Hardware/Core/PageCrossTracker.cpp
    src/Hardware/Core/SymbolTable.cpp
    src/Hardware/Core/SourceCodeCache.cpp
    src/Hardware/Core/DbgFormatParser.cpp
    src/Hardware/Core/DwarfNmParser.cpp
)

target_include_directories(SIM_65C02 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(SIM_65C02 PRIVATE PROJECT_VERSION="${GIT_VERSION}")

if(NOT WIN32 AND NOT EMSCRIPTEN)
    target_link_options(SIM_65C02 PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,--gc-sections
    )
endif()

