include(FetchContent)

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
set(GTEST_URL "https://github.com/google/googletest/archive/refs/heads/main.zip")

# SDL3
set(SDL3_REPO "https://github.com/libsdl-org/SDL.git")
set(SDL3_TAG "release-3.4.x")

# ImGui
set(IMGUI_REPO "https://github.com/ocornut/imgui.git")
set(IMGUI_TAG "docking")

# ImGuiFileDialog
set(IMGUI_FILE_DIALOG_REPO "https://github.com/aiekick/ImGuiFileDialog.git")
set(IMGUI_FILE_DIALOG_TAG "v0.6.8")

# ImGuiColorTextEdit
set(IMGUI_COLOR_TEXT_EDIT_REPO "https://github.com/BalazsJako/ImGuiColorTextEdit.git")
set(IMGUI_COLOR_TEXT_EDIT_TAG "master")

# JSON
set(JSON_URL "https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz")

# PicoSHA2
set(PICOSHA2_URL "https://raw.githubusercontent.com/okdshin/PicoSHA2/master/picosha2.h")

# Glad
set(GLAD_REPO "https://github.com/Dav1dde/glad.git")
set(GLAD_TAG "v2.0.8")

# Miniz
set(MINIZ_REPO "https://github.com/richgel999/miniz.git")
set(MINIZ_TAG "3.1.1")

# SndFile
set(SNDFILE_REPO "https://github.com/libsndfile/libsndfile.git")
set(SNDFILE_TAG "1.2.2")

# FFmpeg (Windows)
set(FFMPEG_WIN_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip")

# ASIO
set(ASIO_URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-38-0.tar.gz")

# cpp-httplib
set(HTTPLIB_REPO "https://github.com/yhirose/cpp-httplib.git")
set(HTTPLIB_TAG "v0.41.0")


# cc65
set(CC65_REPO "https://github.com/cc65/cc65.git")
set(CC65_TAG "V2.19")

# cc65 Snapshot (Windows)
set(CC65_SNAPSHOT_URL "https://sourceforge.net/projects/cc65/files/cc65-snapshot-win32.zip/download")

# ==============================================================================

# Google Test
FetchContent_Declare(
    googletest
    URL ${GTEST_URL}
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# SDL3
set(SDL_SHARED OFF CACHE BOOL "Build shared SDL3" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build static SDL3" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL3 test" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY ${SDL3_REPO}
    GIT_TAG ${SDL3_TAG}
)
set(CMAKE_WARN_DEPRECATED OFF)
FetchContent_MakeAvailable(SDL3)
set(CMAKE_WARN_DEPRECATED ON)

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
FetchContent_MakeAvailable(ImGuiFileDialog)
if(TARGET ImGuiFileDialog)
    target_include_directories(ImGuiFileDialog PUBLIC ${imgui_SOURCE_DIR})
endif()

# ImGuiColorTextEdit
FetchContent_Declare(
    ImGuiColorTextEdit
    GIT_REPOSITORY ${IMGUI_COLOR_TEXT_EDIT_REPO}
    GIT_TAG ${IMGUI_COLOR_TEXT_EDIT_TAG}
)
FetchContent_MakeAvailable(ImGuiColorTextEdit)

# JSON
FetchContent_Declare(
    json
    URL ${JSON_URL}
)
FetchContent_MakeAvailable(json)

# PicoSHA2
FetchContent_Declare(
    picosha2
    URL ${PICOSHA2_URL}
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(picosha2)

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

# SndFile
FetchContent_Declare(
    sndfile
    GIT_REPOSITORY ${SNDFILE_REPO}
    GIT_TAG ${SNDFILE_TAG}
    PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_sndfile.cmake
)

set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_EXTERNAL_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


set(CMAKE_WARN_DEPRECATED OFF)
FetchContent_MakeAvailable(sndfile)
set(CMAKE_WARN_DEPRECATED ON)

if(TARGET sndfile AND NOT TARGET SndFile::sndfile)
    add_library(SndFile::sndfile ALIAS sndfile)
endif()

# Win32 Specifics (OpenSSL/ZLIB)
if(WIN32)
    set(OPENSSL_USE_STATIC_LIBS ON)
    set(ZLIB_USE_STATIC_LIBS ON)
    if(NOT MINGW_ROOT)
        set(MINGW_ROOT "/usr/x86_64-w64-mingw32ucrt/sys-root/mingw")
    endif()
    set(OPENSSL_ROOT_DIR "${MINGW_ROOT}")
    set(ZLIB_ROOT "${MINGW_ROOT}")
endif()

if(NOT EMSCRIPTEN)
    if(WIN32 AND NOT MSVC)
        find_package(OpenSSL REQUIRED)
        find_package(ZLIB REQUIRED)
    else()
        find_package(OpenSSL REQUIRED)
        find_package(ZLIB REQUIRED)
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


# cc65
if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        cc65
        GIT_REPOSITORY ${CC65_REPO}
        GIT_TAG ${CC65_TAG}
    )
    FetchContent_MakeAvailable(cc65)

    FetchContent_Declare(
        cc65_snapshot
        URL ${CC65_SNAPSHOT_URL}
    )
    FetchContent_MakeAvailable(cc65_snapshot)
endif()

if(WIN32 AND NOT MSVC)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()
