# LLVM-MOS Toolchain & Resources Embedding
if(NOT EMSCRIPTEN)
	set(EMBEDDED_LLVM_MOS_H "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_llvm_mos.h")

	add_custom_command(
		OUTPUT "${EMBEDDED_LLVM_MOS_H}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
		COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedLLVMMOS.py" 
				"${LLVM_MOS_ROOT}" 
				"${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker" 
				"${CMAKE_CURRENT_SOURCE_DIR}/sdk/src/Libs" 
				"${EMBEDDED_LLVM_MOS_H}"
				"${CMAKE_CURRENT_SOURCE_DIR}/sdk/microdos/Libs"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedLLVMMOS.py"
				"${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker/link.ld"
				"${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker/bios.s"
				"${CMAKE_CURRENT_SOURCE_DIR}/sdk/microdos/Libs/app-bios.h"
		COMMENT "Generating embedded LLVM-MOS VFS header (embedded_llvm_mos.h)"
		VERBATIM
	)

	add_custom_target(embedded_compilers DEPENDS
		"${EMBEDDED_LLVM_MOS_H}"
	)
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
