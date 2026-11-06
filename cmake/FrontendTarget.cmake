# ImGui Library Target
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL3::SDL3 OpenGL::GL glad_gl)

# Main Frontend Executable
file(GLOB_RECURSE FRONTEND_SOURCES CONFIGURE_DEPENDS
    "src/Frontend/*.cpp"
    "src/Frontend/*.h"
)
list(FILTER FRONTEND_SOURCES EXCLUDE REGEX ".*_(Win32|Posix|Linux|Wasm|Default)\\.cpp$")

if(WIN32)
    add_executable(SIM_65C02 WIN32
        ${FRONTEND_SOURCES}
        "${ICON_RESOURCE_RC}"
        "${SDK_ZIP_H}"
        ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
        ${imguicolortextedit_SOURCE_DIR}/TextEditor.cpp
    )
    target_link_options(SIM_65C02 PRIVATE 
        -static 
        -static-libgcc 
        -static-libstdc++ 
        -Wl,--gc-sections 
        -Wl,--stack,8388608
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(SIM_65C02 PRIVATE -s)
    endif()
    
    target_link_libraries(SIM_65C02 PRIVATE 
        65c02_core 
        imgui_lib 
        mingw32 
        SDL3::SDL3 
        OpenGL::GL 
        glad_gl
        nlohmann_json::nlohmann_json
        httplib::httplib
        OpenSSL::SSL 
        OpenSSL::Crypto
        ws2_32  
        crypt32 
        pathcch
        ${FFMPEG_LIBRARIES}
    )
else()
    add_executable(SIM_65C02 
        ${FRONTEND_SOURCES}
        "${SDK_ZIP_H}"
        ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
        ${imguicolortextedit_SOURCE_DIR}/TextEditor.cpp
    )
    target_link_options(SIM_65C02 PRIVATE 
        -static-libgcc 
        -static-libstdc++ 
        -Wl,--gc-sections 
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(SIM_65C02 PRIVATE -s)
    endif()
    
    target_link_libraries(SIM_65C02 PRIVATE 
        65c02_core 
        imgui_lib 
        SDL3::SDL3 
        OpenGL::GL 
        glad_gl
        nlohmann_json::nlohmann_json
        httplib::httplib
        ${FFMPEG_LIBRARIES}
    )
    if(NOT EMSCRIPTEN)
        target_link_libraries(SIM_65C02 PRIVATE 
            OpenSSL::SSL 
            OpenSSL::Crypto
        )
    endif()
endif()

if(EMSCRIPTEN)
    target_link_options(SIM_65C02 PRIVATE 
        "-sUSE_SDL=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=134217728"
        "-sMAXIMUM_MEMORY=4294967296"
        "-sSTACK_SIZE=8388608"
        "-sMAX_WEBGL_VERSION=2"
        "-pthread"
        "-sPTHREAD_POOL_SIZE=8"
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_main','_Wasm_OnFilePicked','_Wasm_CheckURLHash','_Wasm_LoadSharedData']"
        "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','lengthBytesUTF8','HEAPU8','ccall','cwrap']"
        "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/web/shell.html"
    )
    set_target_properties(SIM_65C02 PROPERTIES SUFFIX ".html")
endif()

target_include_directories(SIM_65C02 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imguifiledialog_SOURCE_DIR}
    ${imguicolortextedit_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${FFMPEG_INCLUDE_DIRS}
)

target_sources(SIM_65C02 PRIVATE "${CASCADIA_MONO_CPP}")
if(WIN32)
    target_sources(SIM_65C02 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Compiler/ProcessRunner_Win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Platform/PlatformUtils_Win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Platform/PlatformHooks_Default.cpp"
    )
elseif(EMSCRIPTEN)
    target_sources(SIM_65C02 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Platform/PlatformUtils_Wasm.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Platform/PlatformHooks_Default.cpp"
    )
else()
    target_sources(SIM_65C02 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Compiler/ProcessRunner_Posix.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Platform/PlatformUtils_Posix.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Platform/PlatformHooks_Linux.cpp"
    )
endif()
add_dependencies(SIM_65C02 crt_shaders app_icons cascadia_mono)
if(NOT EMSCRIPTEN)
    add_dependencies(SIM_65C02 embedded_compilers sdk_resource)
endif()

target_compile_definitions(SIM_65C02 PRIVATE 
    IMGUI_DEFINE_MATH_OPERATORS 
    USE_BOOKMARK 
    CPPHTTPLIB_OPENSSL_SUPPORT
)
target_compile_definitions(SIM_65C02 PRIVATE PROJECT_VERSION="${GIT_VERSION}")

if(SYSTEM_MOS_CLANG)
    target_compile_definitions(SIM_65C02 PRIVATE MOS_CLANG_DEFAULT_PATH="${SYSTEM_MOS_CLANG}")
endif()

