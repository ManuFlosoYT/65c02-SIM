if(WIN32)
    set(FFMPEG_LIBRARIES "")
    FetchContent_Declare(
        ffmpeg_win
        URL ${FFMPEG_WIN_URL}
    )
    FetchContent_MakeAvailable(ffmpeg_win) 
    
    unset(FFMPEG_INC_DIR CACHE)
    foreach(lib avcodec avformat swscale avutil)
        unset(FFMPEG_${lib}_DLL CACHE)
    endforeach()

    find_path(FFMPEG_INC_DIR libavcodec/avcodec.h
        PATHS ${ffmpeg_win_SOURCE_DIR}
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )

    if(NOT CMAKE_DLLTOOL)
        find_program(CMAKE_DLLTOOL NAMES x86_64-w64-mingw32ucrt-dlltool x86_64-w64-mingw32-dlltool dlltool)
    endif()
    if(NOT CMAKE_DLLTOOL)
        message(FATAL_ERROR "dlltool not found, required for FFmpeg delay-import libraries")
    endif()

    set(FFMPEG_DELAY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/delaylibs")
    file(MAKE_DIRECTORY "${FFMPEG_DELAY_DIR}")

    foreach(lib avcodec avformat swscale avutil)
        file(GLOB FFMPEG_${lib}_DEF_LIST "${ffmpeg_win_SOURCE_DIR}/lib/${lib}*.def")
        if(FFMPEG_${lib}_DEF_LIST)
            list(GET FFMPEG_${lib}_DEF_LIST 0 FFMPEG_${lib}_DEF)
        endif()

        file(GLOB FFMPEG_${lib}_DLL_LIST "${ffmpeg_win_SOURCE_DIR}/bin/${lib}*.dll")
        if(FFMPEG_${lib}_DLL_LIST)
            list(GET FFMPEG_${lib}_DLL_LIST 0 FFMPEG_${lib}_DLL)
            set(FFMPEG_${lib}_DLL "${FFMPEG_${lib}_DLL}" CACHE INTERNAL "FFmpeg ${lib} DLL path")
            set(FFMPEG_${lib}_DLL "${FFMPEG_${lib}_DLL}")
        endif()

        if(FFMPEG_${lib}_DEF)
            get_filename_component(DLL_FILENAME "${FFMPEG_${lib}_DLL}" NAME)
            if(NOT DLL_FILENAME)
                get_filename_component(DEF_BASENAME "${FFMPEG_${lib}_DEF}" NAME_WE)
                set(DLL_FILENAME "${DEF_BASENAME}.dll")
            endif()

            set(DELAY_LIB_PATH "${FFMPEG_DELAY_DIR}/lib${lib}.delay.a")
            execute_process(
                COMMAND "${CMAKE_DLLTOOL}" -d "${FFMPEG_${lib}_DEF}" -D "${DLL_FILENAME}" -y "${DELAY_LIB_PATH}"
                RESULT_VARIABLE DLLTOOL_RES
            )
            if(NOT DLLTOOL_RES EQUAL 0)
                message(FATAL_ERROR "dlltool failed to generate delay library for ${lib}")
            endif()

            add_library(FFMPEG::${lib} STATIC IMPORTED)
            set_target_properties(FFMPEG::${lib} PROPERTIES
                IMPORTED_LOCATION "${DELAY_LIB_PATH}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INC_DIR}"
            )
            list(APPEND FFMPEG_LIBRARIES FFMPEG::${lib})
        endif()
    endforeach()

    if(FFMPEG_LIBRARIES)
        list(APPEND FFMPEG_LIBRARIES delayimp)
    endif()
    
    set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INC_DIR}")
else()
    pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libswscale libavutil)
endif()

if(ZLIB_FOUND AND NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${ZLIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}"
    )
endif()
