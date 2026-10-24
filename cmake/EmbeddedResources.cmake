set(EMBED_RESOURCE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_resource.py")

function(target_embed_resource TARGET_NAME)
    cmake_parse_arguments(ARG "" "OUTPUT;COMMENT" "ARGS;DEPENDS" ${ARGN})
    get_filename_component(OUT_DIR "${ARG_OUTPUT}" DIRECTORY)

    add_custom_command(
        OUTPUT "${ARG_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
        COMMAND python3 "${EMBED_RESOURCE_SCRIPT}" ${ARG_ARGS} --output "${ARG_OUTPUT}"
        DEPENDS "${EMBED_RESOURCE_SCRIPT}" ${ARG_DEPENDS}
        COMMENT "${ARG_COMMENT}"
        VERBATIM
    )

    if(NOT "${TARGET_NAME}" STREQUAL "")
        add_custom_target(${TARGET_NAME} DEPENDS "${ARG_OUTPUT}")
    endif()
endfunction()

# LLVM-MOS Toolchain & Resources Embedding
if(NOT EMSCRIPTEN)
    set(EMBEDDED_LLVM_MOS_H "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_llvm_mos.h")
    file(GLOB_RECURSE EMBEDDED_LIBS_DEPS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sdk/src/Libs/*" "${CMAKE_CURRENT_SOURCE_DIR}/sdk/microdos/Libs/*")

    target_embed_resource(embedded_compilers
        OUTPUT "${EMBEDDED_LLVM_MOS_H}"
        ARGS --mode vfs
             --llvm-mos-root "${LLVM_MOS_ROOT}"
             --linker-dir "${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker"
             --libs-dir "${CMAKE_CURRENT_SOURCE_DIR}/sdk/src/Libs"
             --microdos-libs-dir "${CMAKE_CURRENT_SOURCE_DIR}/sdk/microdos/Libs"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker/link.ld"
                "${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker/bios.s"
                ${EMBEDDED_LIBS_DEPS}
        COMMENT "Generating embedded LLVM-MOS VFS header (embedded_llvm_mos.h)"
    )
endif()

# CRT Shaders
set(CRT_VERT "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/GUI/Video/crt.vert")
set(CRT_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/src/Frontend/GUI/Video/crt.frag")
set(CRT_SHADERS_H "${CMAKE_CURRENT_BINARY_DIR}/generated/CRTShaders.h")

target_embed_resource(crt_shaders
    OUTPUT "${CRT_SHADERS_H}"
    ARGS --mode shaders --vert "${CRT_VERT}" --frag "${CRT_FRAG}"
    DEPENDS "${CRT_VERT}" "${CRT_FRAG}"
    COMMENT "Embedding CRT shaders into CRTShaders.h"
)

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

target_embed_resource(""
    OUTPUT "${ICON_PIXELS_H}"
    ARGS --mode icon --magick "${MAGICK_EXECUTABLE}" --input "${ICON_PNG}"
    DEPENDS "${ICON_PNG}"
    COMMENT "Generating SDL3 icon pixel header"
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

    target_embed_resource(sdk_resource
        OUTPUT "${SDK_ZIP_H}"
        ARGS --mode binary --input "${SDK_ZIP}" --var "sdk_zip_data"
        COMMENT "Embedding SDK.zip (if found)"
    )
endif()
