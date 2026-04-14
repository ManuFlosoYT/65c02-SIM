file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "src/Hardware/*.cpp"
    "src/Hardware/*.h"
)

list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Main Core Library
add_library(65c02_core STATIC ${CORE_SOURCES})
target_compile_definitions(65c02_core PUBLIC ASIO_STANDALONE)
if(EMSCRIPTEN)
    target_compile_definitions(65c02_core PUBLIC TARGET_WASM)
endif()
target_include_directories(65c02_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${picosha2_SOURCE_DIR} 
    ${asio_SOURCE_DIR}/asio/include 
    ${miniz_SOURCE_DIR}
)
target_link_libraries(65c02_core PUBLIC 
    SDL3::SDL3 
    httplib::httplib 
    SndFile::sndfile 
    miniz 
    nlohmann_json::nlohmann_json
)
if(NOT EMSCRIPTEN)
    target_link_libraries(65c02_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    if(WIN32)
        target_link_libraries(65c02_core PUBLIC ws2_32 crypt32 pathcch bcrypt)
    endif()
endif()

# Test Variant of Core Library
add_library(65c02_core_test STATIC ${CORE_SOURCES})
target_compile_definitions(65c02_core_test PUBLIC TESTING_ENV ASIO_STANDALONE)
target_include_directories(65c02_core_test PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${picosha2_SOURCE_DIR} 
    ${asio_SOURCE_DIR}/asio/include 
    ${miniz_SOURCE_DIR}
)
target_link_libraries(65c02_core_test PUBLIC 
    SDL3::SDL3 
    httplib::httplib 
    SndFile::sndfile 
    miniz 
    nlohmann_json::nlohmann_json
)
if(NOT EMSCRIPTEN)
    target_link_libraries(65c02_core_test PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    if(WIN32)
        target_link_libraries(65c02_core_test PUBLIC ws2_32 crypt32 pathcch bcrypt)
    endif()
endif()
