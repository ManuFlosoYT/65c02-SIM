file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "src/Backend/*.cpp"
    "src/Backend/*.h"
    "src/Scripting/*.cpp"
    "src/Scripting/*.h"
    "src/Hardware/*.cpp"
    "src/Hardware/*.h"
)

list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*_(Win32|Posix|Linux|Wasm|Default)\\.cpp$")

# Main Core Library
add_library(65c02_core STATIC ${CORE_SOURCES})
target_sources(65c02_core PRIVATE "${SID_WAVETABLES_CPP}")

if(WIN32)
    target_sources(65c02_core PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Hardware/Storage/VirtualFATPlatform_Win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Hardware/Comm/ESP8266/ATPingPlatform_Win32.cpp"
    )
elseif(EMSCRIPTEN)
    target_sources(65c02_core PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Hardware/Storage/VirtualFATPlatform_Posix.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Hardware/Comm/ESP8266/ATPingPlatform_Wasm.cpp"
    )
else()
    target_sources(65c02_core PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Hardware/Storage/VirtualFATPlatform_Posix.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Hardware/Comm/ESP8266/ATPingPlatform_Posix.cpp"
    )
endif()
add_dependencies(65c02_core sid_wavetables)
target_compile_definitions(65c02_core PUBLIC ASIO_STANDALONE)
target_compile_definitions(65c02_core PUBLIC PROJECT_VERSION="${GIT_VERSION}")
if(EMSCRIPTEN)
    target_compile_definitions(65c02_core PUBLIC TARGET_WASM)
endif()
target_include_directories(65c02_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${asio_SOURCE_DIR}/asio/include 
    ${miniz_SOURCE_DIR}
)
target_link_libraries(65c02_core PUBLIC 
    SDL3::SDL3 
    httplib::httplib 
    miniz 
    nlohmann_json::nlohmann_json
)
if(NOT EMSCRIPTEN)
    target_link_libraries(65c02_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    if(WIN32)
        target_link_libraries(65c02_core PUBLIC ws2_32 crypt32 pathcch bcrypt)
    endif()
endif()

target_precompile_headers(65c02_core PRIVATE
    <cstdint>
    <vector>
    <string>
    <memory>
    <algorithm>
    <functional>
    <utility>
)

add_library(65c02_core_test ALIAS 65c02_core)
