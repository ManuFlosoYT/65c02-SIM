# CC65 Embedding
if(NOT EMSCRIPTEN)
    if(NOT TARGET cc65_common)
        file(GLOB CC65_COMMON_SRC "${cc65_SOURCE_DIR}/src/common/*.c")
        add_library(cc65_common STATIC ${CC65_COMMON_SRC})
        target_include_directories(cc65_common PUBLIC "${cc65_SOURCE_DIR}/src/common")

        file(GLOB CC65_CC65_SRC "${cc65_SOURCE_DIR}/src/cc65/*.c")
        add_executable(cc65_bin ${CC65_CC65_SRC})
        target_include_directories(cc65_bin PUBLIC "${cc65_SOURCE_DIR}/src/common")
        target_link_libraries(cc65_bin PRIVATE cc65_common m)

        file(GLOB CC65_CA65_SRC "${cc65_SOURCE_DIR}/src/ca65/*.c")
        add_executable(ca65_bin ${CC65_CA65_SRC})
        target_include_directories(ca65_bin PUBLIC "${cc65_SOURCE_DIR}/src/common")
        target_link_libraries(ca65_bin PRIVATE cc65_common m)

        file(GLOB CC65_LD65_SRC "${cc65_SOURCE_DIR}/src/ld65/*.c")
        add_executable(ld65_bin ${CC65_LD65_SRC})
        target_include_directories(ld65_bin PUBLIC "${cc65_SOURCE_DIR}/src/common")
        target_link_libraries(ld65_bin PRIVATE cc65_common m)
        
        set_target_properties(cc65_bin ca65_bin ld65_bin PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

        # Generate C++ headers containing the executable bytes
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/cc65_exe.h"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
            COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py" "$<TARGET_FILE:cc65_bin>" "cc65_exe_data" "${CMAKE_CURRENT_BINARY_DIR}/generated/cc65_exe.h"
            DEPENDS cc65_bin "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py"
            COMMENT "Embedding cc65_bin executable"
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/ca65_exe.h"
            COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py" "$<TARGET_FILE:ca65_bin>" "ca65_exe_data" "${CMAKE_CURRENT_BINARY_DIR}/generated/ca65_exe.h"
            DEPENDS ca65_bin "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py"
            COMMENT "Embedding ca65_bin executable"
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/ld65_exe.h"
            COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py" "$<TARGET_FILE:ld65_bin>" "ld65_exe_data" "${CMAKE_CURRENT_BINARY_DIR}/generated/ld65_exe.h"
            DEPENDS ld65_bin "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py"
            COMMENT "Embedding ld65_bin executable"
            VERBATIM
        )

        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_cc65.h"
            COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedCC65.py" "${cc65_snapshot_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker" "${CMAKE_CURRENT_SOURCE_DIR}/sdk/src/Libs" "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_cc65.h"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedCC65.py"
            COMMENT "Embedding CC65 VFS contents (headers and none.lib)"
            VERBATIM
        )

        add_custom_target(embedded_compilers DEPENDS
            "${CMAKE_CURRENT_BINARY_DIR}/generated/cc65_exe.h"
            "${CMAKE_CURRENT_BINARY_DIR}/generated/ca65_exe.h"
            "${CMAKE_CURRENT_BINARY_DIR}/generated/ld65_exe.h"
            "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_cc65.h"
        )
    endif()
endif()

# CRT Shaders
set(CRT_VERT "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/GUI/Video/crt.vert")
set(CRT_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/GUI/Video/crt.frag")
set(CRT_SHADERS_H "${CMAKE_CURRENT_BINARY_DIR}/generated/CRTShaders.h")

add_custom_command(
    OUTPUT  "${CRT_SHADERS_H}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND ${CMAKE_COMMAND}
        -DVERT_FILE=${CRT_VERT}
        -DFRAG_FILE=${CRT_FRAG}
        -DOUTPUT=${CRT_SHADERS_H}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedCRTShaders.cmake"
    DEPENDS "${CRT_VERT}" "${CRT_FRAG}"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedCRTShaders.cmake"
    COMMENT "Embedding CRT shaders into CRTShaders.h"
    VERBATIM
)
add_custom_target(crt_shaders DEPENDS "${CRT_SHADERS_H}")

# App Icon Generation
find_program(RSVG_CONVERT_EXECUTABLE NAMES rsvg-convert)
find_program(MAGICK_EXECUTABLE NAMES magick convert REQUIRED)

set(ICON_SVG "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/Assets/65c02-sim.svg")
set(ICON_PNG "${CMAKE_CURRENT_BINARY_DIR}/generated/65c02-sim.png")
set(ICON_ICO "${CMAKE_CURRENT_BINARY_DIR}/generated/65c02-sim.ico")
set(ICON_PIXELS_H "${CMAKE_CURRENT_BINARY_DIR}/generated/IconPixels.h")
set(ICON_RESOURCE_RC "${CMAKE_CURRENT_BINARY_DIR}/generated/resource.rc")

if(RSVG_CONVERT_EXECUTABLE)
    add_custom_command(
        OUTPUT "${ICON_PNG}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND ${RSVG_CONVERT_EXECUTABLE} --width=256 --height=256 --keep-aspect-ratio "${ICON_SVG}" --output "${ICON_PNG}"
        DEPENDS "${ICON_SVG}"
        COMMENT "Generating PNG icon from SVG (using rsvg-convert)"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT "${ICON_PNG}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND ${MAGICK_EXECUTABLE} -background none "${ICON_SVG}" -resize 256x256 "${ICON_PNG}"
        DEPENDS "${ICON_SVG}"
        COMMENT "Generating PNG icon from SVG (using ImageMagick)"
        VERBATIM
    )
endif()

add_custom_command(
    OUTPUT "${ICON_ICO}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND ${MAGICK_EXECUTABLE} "${ICON_PNG}" -define icon:auto-resize=256,128,64,48,32,16 "${ICON_ICO}"
    DEPENDS "${ICON_PNG}"
    COMMENT "Generating Windows ICO from PNG"
    VERBATIM
)

add_custom_command(
    OUTPUT "${ICON_PIXELS_H}"
    COMMAND ${CMAKE_COMMAND}
        -DMAGICK_EXECUTABLE=${MAGICK_EXECUTABLE}
        -DINPUT_PNG=${ICON_PNG}
        -DOUTPUT_HEADER=${ICON_PIXELS_H}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateIconHeader.cmake"
    DEPENDS "${ICON_PNG}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateIconHeader.cmake"
    COMMENT "Generating SDL3 icon pixel header"
    VERBATIM
)

add_custom_command(
    OUTPUT "${ICON_RESOURCE_RC}"
    COMMAND ${CMAKE_COMMAND} -E echo "IDI_ICON1 ICON DISCARDABLE \"${ICON_ICO}\"" > "${ICON_RESOURCE_RC}"
    DEPENDS "${ICON_ICO}"
    COMMENT "Generating Windows resource file"
    VERBATIM
)

add_custom_target(app_icons DEPENDS "${ICON_PIXELS_H}" "${ICON_RESOURCE_RC}")

# SDK Embedding
if(NOT EMSCRIPTEN)
    set(SDK_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/SDK.zip")
    set(SDK_ZIP_H "${CMAKE_CURRENT_BINARY_DIR}/generated/sdk_zip.h")

    # The script now handles missing SDK.zip by creating an empty vector at build-time.
    add_custom_command(
        OUTPUT "${SDK_ZIP_H}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py" "${SDK_ZIP}" "sdk_zip_data" "${SDK_ZIP_H}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.py"
        COMMENT "Embedding SDK.zip (if found)"
        VERBATIM
    )
    add_custom_target(sdk_resource DEPENDS "${SDK_ZIP_H}")
endif()
