include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable network updates for FetchContent dependencies" FORCE)

option(BUILD_UNIT_TESTS "Build unit tests" ON)
option(USE_SYSTEM_SDL3 "Use system SDL3 instead of FetchContent" OFF)
option(BUILD_SDK_BUNDLE "Build SDK.zip from source via create-sdk.sh" OFF)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/_flatpak_deps")
    foreach(_dep_pair
        "GOOGLETEST:googletest"
        "SDL3:sdl3"
        "IMGUI:imgui"
        "IMGUIFILEDIALOG:imguifiledialog"
        "IMGUICOLORTEXTEDIT:imguicolortextedit"
        "JSON:json"
        "GLAD:glad"
        "MINIZ:miniz"
        "ASIO:asio"
        "HTTPLIB:httplib"
    )
        string(REPLACE ":" ";" _dep_parts "${_dep_pair}")
        list(GET _dep_parts 0 _dep_upper)
        list(GET _dep_parts 1 _dep_dir)
        if(NOT DEFINED FETCHCONTENT_SOURCE_DIR_${_dep_upper} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/_flatpak_deps/${_dep_dir}")
            set(FETCHCONTENT_SOURCE_DIR_${_dep_upper} "${CMAKE_CURRENT_SOURCE_DIR}/_flatpak_deps/${_dep_dir}" CACHE PATH "" FORCE)
        endif()
    endforeach()
endif()

if(WIN32 AND NOT MSVC)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(PKG_CONFIG_EXECUTABLE "" CACHE FILEPATH "Disabled for Windows cross-build" FORCE)
endif()

# ==============================================================================
# Dependency Versions & URLs
# ==============================================================================

# Google Test
set(GTEST_URL "https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip")

# SDL3
set(SDL3_REPO "https://github.com/libsdl-org/SDL.git")
set(SDL3_TAG "release-3.4.20")

# ImGui
set(IMGUI_REPO "https://github.com/ocornut/imgui.git")
set(IMGUI_TAG "v1.93.0")

# ImGuiFileDialog
set(IMGUI_FILE_DIALOG_REPO "https://github.com/aiekick/ImGuiFileDialog.git")
set(IMGUI_FILE_DIALOG_TAG "v0.6.8")

# ImGuiColorTextEdit
set(IMGUI_COLOR_TEXT_EDIT_REPO "https://github.com/BalazsJako/ImGuiColorTextEdit.git")
set(IMGUI_COLOR_TEXT_EDIT_TAG "ca2f9f1462e3b60e56351bc466acda448c5ea50d")

# JSON
set(JSON_URL "https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz")

# Glad
set(GLAD_REPO "https://github.com/Dav1dde/glad.git")
set(GLAD_TAG "v2.0.8")

# Miniz
set(MINIZ_REPO "https://github.com/richgel999/miniz.git")
set(MINIZ_TAG "3.1.2")

# FFmpeg (Windows)
set(FFMPEG_WIN_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip")

# ASIO
set(ASIO_URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-39-0.tar.gz")

# cpp-httplib
set(HTTPLIB_REPO "https://github.com/yhirose/cpp-httplib.git")
set(HTTPLIB_TAG "v0.66.0")


# LLVM-MOS SDK
set(LLVM_MOS_TAG "v23.4.0")
if(WIN32)
    set(LLVM_MOS_URL "https://github.com/llvm-mos/llvm-mos-sdk/releases/download/${LLVM_MOS_TAG}/llvm-mos-windows.7z")
else()
    set(LLVM_MOS_URL "https://github.com/llvm-mos/llvm-mos-sdk/releases/download/${LLVM_MOS_TAG}/llvm-mos-linux.tar.xz")
endif()

# ==============================================================================

set(DEPS_VERSION_STAMP "${GTEST_URL}|${SDL3_TAG}|${IMGUI_TAG}|${IMGUI_FILE_DIALOG_TAG}|${IMGUI_COLOR_TEXT_EDIT_TAG}|${JSON_URL}|${GLAD_TAG}|${MINIZ_TAG}|${ASIO_URL}|${HTTPLIB_TAG}|${LLVM_MOS_TAG}")
if(NOT FETCHCONTENT_FULLY_DISCONNECTED AND NOT "${LAST_DEPS_VERSION_STAMP}" STREQUAL "${DEPS_VERSION_STAMP}")
    message(STATUS "Dependency versions changed; enabling FetchContent updates for this configure...")
    set(FETCHCONTENT_UPDATES_DISCONNECTED OFF CACHE BOOL "Disable network updates for FetchContent dependencies" FORCE)
endif()

# Google Test
if(BUILD_UNIT_TESTS)
    FetchContent_Declare(
        googletest
        URL ${GTEST_URL}
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    if(WIN32)
        target_compile_definitions(gtest PUBLIC GTEST_API_=)
    endif()
endif()

# SDL3
if(USE_SYSTEM_SDL3)
    find_package(SDL3 REQUIRED CONFIG)
else()
    set(SDL_SHARED OFF CACHE BOOL "Build shared SDL3" FORCE)
    set(SDL_STATIC ON CACHE BOOL "Build static SDL3" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL3 test library" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "Build SDL3 tests" FORCE)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY ${SDL3_REPO}
        GIT_TAG ${SDL3_TAG}
        PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_sdl3.cmake
    )
    set(CMAKE_WARN_DEPRECATED OFF)
    set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
    if(DEFINED FETCHCONTENT_SOURCE_DIR_SDL3 AND EXISTS "${FETCHCONTENT_SOURCE_DIR_SDL3}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_sdl3.cmake"
            WORKING_DIRECTORY "${FETCHCONTENT_SOURCE_DIR_SDL3}"
        )
    endif()
    FetchContent_MakeAvailable(SDL3)
    set(CMAKE_DISABLE_PRECOMPILE_HEADERS OFF)
    set(CMAKE_WARN_DEPRECATED ON)
endif()

# ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY ${IMGUI_REPO}
    GIT_TAG ${IMGUI_TAG}
)
FetchContent_MakeAvailable(imgui)

# ImGuiFileDialog
FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY ${IMGUI_FILE_DIALOG_REPO}
    GIT_TAG ${IMGUI_FILE_DIALOG_TAG}
    PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_imguifiledialog.cmake
)
if(DEFINED FETCHCONTENT_SOURCE_DIR_IMGUIFILEDIALOG AND EXISTS "${FETCHCONTENT_SOURCE_DIR_IMGUIFILEDIALOG}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_imguifiledialog.cmake"
        WORKING_DIRECTORY "${FETCHCONTENT_SOURCE_DIR_IMGUIFILEDIALOG}"
    )
endif()
FetchContent_MakeAvailable(ImGuiFileDialog)
if(TARGET ImGuiFileDialog)
    target_include_directories(ImGuiFileDialog PUBLIC ${imgui_SOURCE_DIR})
endif()

# ImGuiColorTextEdit
FetchContent_Declare(
    ImGuiColorTextEdit
    GIT_REPOSITORY ${IMGUI_COLOR_TEXT_EDIT_REPO}
    GIT_TAG ${IMGUI_COLOR_TEXT_EDIT_TAG}
    PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_imguicolortextedit.cmake
)
if(DEFINED FETCHCONTENT_SOURCE_DIR_IMGUICOLORTEXTEDIT AND EXISTS "${FETCHCONTENT_SOURCE_DIR_IMGUICOLORTEXTEDIT}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_imguicolortextedit.cmake"
        WORKING_DIRECTORY "${FETCHCONTENT_SOURCE_DIR_IMGUICOLORTEXTEDIT}"
    )
endif()
FetchContent_MakeAvailable(ImGuiColorTextEdit)

# JSON
FetchContent_Declare(
    json
    URL ${JSON_URL}
)
FetchContent_MakeAvailable(json)

# Glad
FetchContent_Declare(
    glad
    GIT_REPOSITORY ${GLAD_REPO}
    GIT_TAG ${GLAD_TAG}
    SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glad)
include(cmake/GladExtensions.cmake)

# Miniz
FetchContent_Declare(
    miniz
    GIT_REPOSITORY ${MINIZ_REPO}
    GIT_TAG ${MINIZ_TAG}
    PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_miniz.cmake
)
if(DEFINED FETCHCONTENT_SOURCE_DIR_MINIZ AND EXISTS "${FETCHCONTENT_SOURCE_DIR_MINIZ}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_miniz.cmake"
        WORKING_DIRECTORY "${FETCHCONTENT_SOURCE_DIR_MINIZ}"
    )
endif()
FetchContent_MakeAvailable(miniz)

# OpenGL
if(WIN32 AND NOT MSVC)
    if(NOT TARGET OpenGL::GL)
        add_library(OpenGL::GL INTERFACE IMPORTED)
        target_link_libraries(OpenGL::GL INTERFACE opengl32)
    endif()
else()
    find_package(OpenGL REQUIRED)
endif()

# Win32 Specifics (OpenSSL)
if(WIN32)
    set(OPENSSL_USE_STATIC_LIBS ON)
    if(NOT MINGW_ROOT)
        set(MINGW_ROOT "/usr/x86_64-w64-mingw32ucrt/sys-root/mingw")
    endif()
    set(OPENSSL_ROOT_DIR "${MINGW_ROOT}")
endif()

if(NOT EMSCRIPTEN)
    if(WIN32 AND NOT MSVC)
        find_package(OpenSSL REQUIRED)
    else()
        find_package(OpenSSL REQUIRED)
        find_package(PkgConfig REQUIRED)
    endif()
endif()


# FFmpeg Logic
if(NOT EMSCRIPTEN)
    include(cmake/FindFFmpegWindows.cmake)
endif()

# ASIO
FetchContent_Declare(
    asio
    URL ${ASIO_URL}
)
FetchContent_MakeAvailable(asio)

# cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY ${HTTPLIB_REPO}
    GIT_TAG ${HTTPLIB_TAG}
)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE) 
set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
if(WIN32)
    set(HTTPLIB_USE_NON_BLOCKING_GETADDRINFO OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(httplib)
if(TARGET httplib AND NOT EMSCRIPTEN)
    target_compile_definitions(httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(httplib INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif()


# LLVM-MOS SDK
if(NOT EMSCRIPTEN)
    set(LLVM_MOS_HINTS
        ENV LLVM_MOS_DIR
        ENV LLVM_MOS_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/build/llvm_mos/llvm-mos/bin"
        "${CMAKE_CURRENT_SOURCE_DIR}/output/toolchain/bin"
    )
    if(DEFINED ENV{MOS_CLANG} AND EXISTS "$ENV{MOS_CLANG}")
        get_filename_component(_env_mos_bin_dir "$ENV{MOS_CLANG}" DIRECTORY)
        list(PREPEND LLVM_MOS_HINTS "${_env_mos_bin_dir}")
    endif()
    if(MOS_CLANG AND EXISTS "${MOS_CLANG}")
        get_filename_component(_arg_mos_bin_dir "${MOS_CLANG}" DIRECTORY)
        list(PREPEND LLVM_MOS_HINTS "${_arg_mos_bin_dir}")
    endif()
    if(LLVM_MOS_DIR)
        list(PREPEND LLVM_MOS_HINTS "${LLVM_MOS_DIR}" "${LLVM_MOS_DIR}/bin")
    endif()
    find_program(SYSTEM_MOS_CLANG mos-common-clang HINTS ${LLVM_MOS_HINTS} PATH_SUFFIXES bin)
    if(NOT SYSTEM_MOS_CLANG)
        find_program(SYSTEM_MOS_CLANG mos-clang HINTS ${LLVM_MOS_HINTS} PATH_SUFFIXES bin)
    endif()

    if(SYSTEM_MOS_CLANG)
        message(STATUS "Found LLVM-MOS compiler: ${SYSTEM_MOS_CLANG}")
        get_filename_component(MOS_BIN_DIR "${SYSTEM_MOS_CLANG}" DIRECTORY)
        get_filename_component(LLVM_MOS_ROOT "${MOS_BIN_DIR}" DIRECTORY)
    else()
        FetchContent_Declare(
            llvm_mos
            URL ${LLVM_MOS_URL}
        )
        FetchContent_MakeAvailable(llvm_mos)
        find_program(SYSTEM_MOS_CLANG
            NAMES mos-common-clang mos-clang
            HINTS "${llvm_mos_SOURCE_DIR}" "${llvm_mos_SOURCE_DIR}/llvm-mos"
            PATH_SUFFIXES bin
            NO_DEFAULT_PATH
        )
        if(SYSTEM_MOS_CLANG)
            message(STATUS "Found fetched LLVM-MOS compiler: ${SYSTEM_MOS_CLANG}")
            get_filename_component(MOS_BIN_DIR "${SYSTEM_MOS_CLANG}" DIRECTORY)
            get_filename_component(LLVM_MOS_ROOT "${MOS_BIN_DIR}" DIRECTORY)
        else()
            if(EXISTS "${llvm_mos_SOURCE_DIR}/llvm-mos/bin")
                set(LLVM_MOS_ROOT "${llvm_mos_SOURCE_DIR}/llvm-mos")
            else()
                set(LLVM_MOS_ROOT "${llvm_mos_SOURCE_DIR}")
            endif()
            set(MOS_BIN_DIR "${LLVM_MOS_ROOT}/bin")
            if(EXISTS "${MOS_BIN_DIR}/mos-common-clang${CMAKE_EXECUTABLE_SUFFIX}")
                set(SYSTEM_MOS_CLANG "${MOS_BIN_DIR}/mos-common-clang${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "Path to LLVM-MOS compiler" FORCE)
            elseif(EXISTS "${MOS_BIN_DIR}/mos-clang${CMAKE_EXECUTABLE_SUFFIX}")
                set(SYSTEM_MOS_CLANG "${MOS_BIN_DIR}/mos-clang${CMAKE_EXECUTABLE_SUFFIX}" CACHE FILEPATH "Path to LLVM-MOS compiler" FORCE)
            endif()
        endif()
    endif()

    if(SYSTEM_MOS_CLANG)
        set(MOS_CLANG "${SYSTEM_MOS_CLANG}")
    endif()
endif()

if(WIN32 AND NOT MSVC)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()

set(LAST_DEPS_VERSION_STAMP "${DEPS_VERSION_STAMP}" CACHE INTERNAL "Last configured dependency version stamp")
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable network updates for FetchContent dependencies" FORCE)
