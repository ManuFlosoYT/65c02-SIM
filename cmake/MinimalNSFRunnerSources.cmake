set(MINIMAL_NSF_RUNNER_SOURCES
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
    src/Hardware/Core/Logger.cpp
)

function(configure_minimal_nsf_runner_target TARGET_NAME)
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE PROJECT_VERSION="${GIT_VERSION}")

    if(NOT WIN32 AND NOT EMSCRIPTEN)
        if(HAVE_STATIC_LIBSTDCXX)
            target_link_options(${TARGET_NAME} PRIVATE
                -static-libgcc
                -static-libstdc++
            )
        endif()
        target_link_options(${TARGET_NAME} PRIVATE -Wl,--gc-sections)
    endif()
endfunction()
