set(SDL_VULKAN_RENDERER_FILE "src/render/vulkan/SDL_render_vulkan.c")

if(EXISTS "${SDL_VULKAN_RENDERER_FILE}")
    file(READ "${SDL_VULKAN_RENDERER_FILE}" CONTENT)
    if(CONTENT MATCHES "const uint32_t YUV_SD_THRESHOLD = 576;")
        string(REPLACE
            "        const uint32_t YUV_SD_THRESHOLD = 576;\n"
            ""
            CONTENT
            "${CONTENT}"
        )
        file(WRITE "${SDL_VULKAN_RENDERER_FILE}" "${CONTENT}")
    endif()
endif()
