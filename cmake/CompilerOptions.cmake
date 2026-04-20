include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
option(PERFDEBUG_ENABLE_LTO "Enable LTO for PerfDebug builds" OFF)
option(PERF_TUNE_NATIVE "Enable -march=native -mtune=native for local builds" OFF)
option(PGO_GENERATE "Enable profile-guided optimization instrumentation" OFF)
option(PGO_USE "Use profile-guided optimization data" OFF)
set(PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo-data" CACHE STRING "Directory used to generate/read PGO profiles")

if(PGO_GENERATE AND PGO_USE)
    message(FATAL_ERROR "PGO_GENERATE and PGO_USE cannot be enabled at the same time")
endif()

if(lto_supported)
    message(STATUS "LTO (IPO) is compatible")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    if(PERFDEBUG_ENABLE_LTO)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_PERFDEBUG ON)
        message(STATUS "LTO (IPO) enabled for PerfDebug")
    endif()
else()
    message(STATUS "LTO (IPO) is not compatible: ${error}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

set(TUNE_NATIVE_FLAGS "")
set(PGO_COMPILE_FLAGS "")
set(PGO_LINK_FLAGS "")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(PERF_TUNE_NATIVE)
        set(TUNE_NATIVE_FLAGS " -march=native -mtune=native")
    endif()

    if(PGO_GENERATE)
        set(PGO_COMPILE_FLAGS " -fprofile-generate=${PGO_DATA_DIR}")
        set(PGO_LINK_FLAGS " -fprofile-generate=${PGO_DATA_DIR}")
    elseif(PGO_USE)
        set(PGO_COMPILE_FLAGS " -fprofile-use=${PGO_DATA_DIR} -fprofile-correction")
        set(PGO_LINK_FLAGS " -fprofile-use=${PGO_DATA_DIR} -fprofile-correction")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -ffunction-sections -fdata-sections${TUNE_NATIVE_FLAGS}${PGO_COMPILE_FLAGS}")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffunction-sections -fdata-sections -falign-functions=32 -falign-loops=32 -falign-jumps=32 -falign-labels=32${TUNE_NATIVE_FLAGS}${PGO_COMPILE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -Wl,--gc-sections -s${PGO_LINK_FLAGS}")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "PerfDebug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS_PERFDEBUG "-O3 -g -fno-omit-frame-pointer -DNDEBUG -ffunction-sections -fdata-sections${TUNE_NATIVE_FLAGS}${PGO_COMPILE_FLAGS}")
        set(CMAKE_CXX_FLAGS_PERFDEBUG "-O3 -g -fno-omit-frame-pointer -DNDEBUG -ffunction-sections -fdata-sections -falign-functions=32 -falign-loops=32 -falign-jumps=32 -falign-labels=32${TUNE_NATIVE_FLAGS}${PGO_COMPILE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS_PERFDEBUG "${CMAKE_EXE_LINKER_FLAGS_PERFDEBUG} -Wl,--gc-sections${PGO_LINK_FLAGS}")
        if(PERFDEBUG_ENABLE_LTO)
            set(CMAKE_EXE_LINKER_FLAGS_PERFDEBUG "${CMAKE_EXE_LINKER_FLAGS_PERFDEBUG} -flto")
        endif()
    endif()
endif()

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()

if(EMSCRIPTEN)
    add_compile_definitions(TARGET_WASM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sUSE_PTHREADS=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_PTHREADS=1")
    add_link_options("-sUSE_PTHREADS=1")
    add_link_options("-sPTHREAD_POOL_SIZE=8")
    add_link_options("-sALLOW_MEMORY_GROWTH=1")
    add_link_options("-sINITIAL_MEMORY=134217728") # 128MB
    add_link_options("-sMAXIMUM_MEMORY=4294967296") # 4GB
    add_link_options("-sSTACK_SIZE=8388608") # 8MB
    add_link_options("-sMAX_WEBGL_VERSION=2")
    # Optimize for WASM
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Versioning logic
find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_VERSION "v0.0.0")
endif()

if("${GIT_VERSION}" STREQUAL "")
    set(GIT_VERSION "v0.0.0")
endif()

add_compile_definitions(PROJECT_VERSION="${GIT_VERSION}")
