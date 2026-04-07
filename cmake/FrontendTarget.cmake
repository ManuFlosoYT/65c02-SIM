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
    "Frontend/*.cpp"
    "Frontend/*.h"
)

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
        ZLIB::ZLIB
        ws2_32  
        crypt32 
        pathcch
        ${FFMPEG_LIBRARIES}
    )

    if(FFMPEG_LIBRARIES)
        foreach(lib_target ${FFMPEG_LIBRARIES})
            get_target_property(dll_path ${lib_target} IMPORTED_LOCATION)
            if(dll_path)
                add_custom_command(TARGET SIM_65C02 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll_path}"
                    "$<TARGET_FILE_DIR:SIM_65C02>/"
                )
            endif()
        endforeach()
    endif()
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
            ZLIB::ZLIB
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
        "-sUSE_PTHREADS=1"
        "-sPTHREAD_POOL_SIZE=8"
        "-sEXPORTED_FUNCTIONS=['_malloc','_free','_main','_Wasm_OnFilePicked']"
        "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','lengthBytesUTF8','HEAPU8','ccall','cwrap']"
        "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/Frontend/web/shell.html"
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

add_dependencies(SIM_65C02 crt_shaders app_icons)
if(NOT EMSCRIPTEN)
    add_dependencies(SIM_65C02 embedded_compilers sdk_resource)
endif()

target_compile_definitions(SIM_65C02 PRIVATE 
    IMGUI_DEFINE_MATH_OPERATORS 
    USE_BOOKMARK 
    CPPHTTPLIB_OPENSSL_SUPPORT
)
