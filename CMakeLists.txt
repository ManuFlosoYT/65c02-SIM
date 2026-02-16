cmake_minimum_required(VERSION 3.24)
project(SIM_65C02)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)

if(lto_supported)
    message(STATUS "LTO (IPO) es compatible y ha sido activado")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
    message(STATUS "LTO no es compatible: ${error}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffunction-sections -fdata-sections")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto -Wl,--gc-sections -s")
    endif()
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glew)

FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG v0.6.4
)
FetchContent_MakeAvailable(ImGuiFileDialog)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

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

#set(GIT_VERSION "v0.6.7") # FORCED FOR TESTING

add_compile_definitions(PROJECT_VERSION="${GIT_VERSION}")

find_package(OpenGL REQUIRED)

if(WIN32)
    if(NOT MINGW_ROOT)
        set(MINGW_ROOT "/usr/x86_64-w64-mingw32/sys-root/mingw")
    endif()
    set(OPENSSL_ROOT_DIR "${MINGW_ROOT}")
    set(OPENSSL_USE_STATIC_LIBS ON)
    set(ZLIB_ROOT "${MINGW_ROOT}")
    set(ZLIB_USE_STATIC_LIBS ON)
endif()

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

if(ZLIB_FOUND AND NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${ZLIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}"
    )
endif()

message(STATUS "ZLib Found: ${ZLIB_LIBRARIES}")
message(STATUS "OpenSSL Found: ${OPENSSL_LIBRARIES}")

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE) 
set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(httplib)
if(TARGET httplib)
    target_compile_definitions(httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(httplib INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif()

file(GLOB CORE_SOURCES
    "Hardware/CPU/*.cpp"
    "Hardware/CPU/*.h"
    "Hardware/*.cpp"
    "Hardware/*.h"
)

list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(65c02_core STATIC ${CORE_SOURCES})
target_include_directories(65c02_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(65c02_core PUBLIC SDL2::SDL2)

add_library(65c02_core_test STATIC ${CORE_SOURCES})
target_compile_definitions(65c02_core_test PUBLIC TESTING_ENV)
target_include_directories(65c02_core_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(65c02_core_test PUBLIC SDL2::SDL2)

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

file(GLOB_RECURSE FRONTEND_SOURCES
    "Frontend/*.cpp"
    "Frontend/*.h"
)

if(WIN32)
    add_executable(SIM_65C02 WIN32
        ${FRONTEND_SOURCES}
        ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
    )
    target_link_options(SIM_65C02 PRIVATE 
        -static 
        -static-libgcc 
        -static-libstdc++ 
        -Wl,--gc-sections 
        -s
    )
    
    target_link_libraries(SIM_65C02 PRIVATE 
        65c02_core 
        imgui_lib 
        mingw32 
        SDL2::SDL2main 
        SDL2::SDL2 
        libglew_static 
        OpenGL::GL 
        nlohmann_json::nlohmann_json
        httplib::httplib
        OpenSSL::SSL 
        OpenSSL::Crypto
        ZLIB::ZLIB
        ws2_32  
        crypt32 
        pathcch
    )
else()
    add_executable(SIM_65C02 
        ${FRONTEND_SOURCES}
        ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
    )
    target_link_options(SIM_65C02 PRIVATE 
        -static-libgcc 
        -static-libstdc++ 
        -Wl,--gc-sections 
        -s
    )
    
    target_link_libraries(SIM_65C02 PRIVATE 
        65c02_core 
        imgui_lib 
        SDL2::SDL2 
        libglew_static 
        OpenGL::GL 
        nlohmann_json::nlohmann_json
        httplib::httplib
        OpenSSL::SSL 
        OpenSSL::Crypto
        ZLIB::ZLIB
    )
endif()

target_include_directories(SIM_65C02 PRIVATE ${glew_SOURCE_DIR}/include ${imguifiledialog_SOURCE_DIR})

target_compile_definitions(SIM_65C02 PRIVATE 
    IMGUI_DEFINE_MATH_OPERATORS 
    USE_BOOKMARK 
    CPPHTTPLIB_OPENSSL_SUPPORT
)

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
