cmake_minimum_required(VERSION 3.24)
project(SIM_65C02)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Force -O3 optimization for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    endif()
endif()

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------
include(FetchContent)

# 1. GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. SDL2
# Build SDL2 as a static library
set(SDL_SHARED OFF CACHE BOOL "Build shared SDL2" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build static SDL2" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL2 test" FORCE)

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.32.x
)
set(CMAKE_WARN_DEPRECATED OFF)
FetchContent_MakeAvailable(SDL2)
set(CMAKE_WARN_DEPRECATED ON)

# 3. Dear ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# 4. GLEW (FetchContent)
FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glew)

# 5. ImGuiFileDialog
FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG v0.6.4
)
FetchContent_MakeAvailable(ImGuiFileDialog)

# 6. nlohmann_json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Get version from Git
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

# Fallback if git describe failed or returned empty
if("${GIT_VERSION}" STREQUAL "")
    set(GIT_VERSION "v0.0.0")
endif()

#set(GIT_VERSION "v1.0.0") # FORCED FOR TESTING

add_compile_definitions(PROJECT_VERSION="${GIT_VERSION}")

find_package(OpenGL REQUIRED)

# ------------------------------------------------------------------------------
# Core Library
# ------------------------------------------------------------------------------
file(GLOB CORE_SOURCES
    "Hardware/CPU/*.cpp"
    "Hardware/CPU/*.h"
    "Hardware/*.cpp"
    "Hardware/*.h"
)
# Exclude main.cpp if it gets globbed (it is not in subdirs so likely fine, GLOB above is restrictive)
# But just in case
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(65c02_core STATIC ${CORE_SOURCES})
target_include_directories(65c02_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(65c02_core PUBLIC SDL2::SDL2)

# Test version of core (for unit tests if needed, though usually same lib works)
add_library(65c02_core_test STATIC ${CORE_SOURCES})
target_compile_definitions(65c02_core_test PUBLIC TESTING_ENV)
target_include_directories(65c02_core_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(65c02_core_test PUBLIC SDL2::SDL2)

# ------------------------------------------------------------------------------
# ImGui Library (Static helper)
# ------------------------------------------------------------------------------
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
    ${glew_SOURCE_DIR}/include
)
target_link_libraries(imgui_lib PUBLIC SDL2::SDL2 libglew_static OpenGL::GL)

# ------------------------------------------------------------------------------
# Target: Simulator Application
# ------------------------------------------------------------------------------
file(GLOB_RECURSE FRONTEND_SOURCES
    "Frontend/*.cpp"
    "Frontend/*.h"
)

if(WIN32)
    add_executable(SIM_65C02 WIN32
        ${FRONTEND_SOURCES}
        ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
    )
    # Static linking for Windows to avoid missing DLLs
    target_link_options(SIM_65C02 PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_libraries(SIM_65C02 PRIVATE 65c02_core imgui_lib mingw32 SDL2::SDL2main SDL2::SDL2 libglew_static OpenGL::GL nlohmann_json::nlohmann_json)
else()
    add_executable(SIM_65C02 
        ${FRONTEND_SOURCES}
        ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
    )
    # Static runtime linking for Linux (optional but good for portability)
    target_link_options(SIM_65C02 PRIVATE -static-libgcc -static-libstdc++)
    target_link_libraries(SIM_65C02 PRIVATE 65c02_core imgui_lib SDL2::SDL2 libglew_static OpenGL::GL nlohmann_json::nlohmann_json)
endif()

target_include_directories(SIM_65C02 PRIVATE ${glew_SOURCE_DIR}/include ${imguifiledialog_SOURCE_DIR})
target_compile_definitions(SIM_65C02 PRIVATE IMGUI_DEFINE_MATH_OPERATORS USE_BOOKMARK)

# ------------------------------------------------------------------------------
# Target 3: Unit Tests
# ------------------------------------------------------------------------------
enable_testing()
file(GLOB_RECURSE TEST_SOURCES "UnitTests/*.cpp")

if(TEST_SOURCES)
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
