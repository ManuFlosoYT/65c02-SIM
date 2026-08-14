if(WIN32)
    set(FFMPEG_LIBRARIES "")
    FetchContent_Declare(
        ffmpeg_win
        URL ${FFMPEG_WIN_URL}
    )
    FetchContent_MakeAvailable(ffmpeg_win) 
    
    unset(FFMPEG_INC_DIR CACHE)
    foreach(lib avcodec avformat swscale avutil)
        unset(FFMPEG_${lib}_LIB CACHE)
        unset(FFMPEG_${lib}_DLL CACHE)
    endforeach()

    find_path(FFMPEG_INC_DIR libavcodec/avcodec.h
        PATHS ${ffmpeg_win_SOURCE_DIR}
        PATH_SUFFIXES include
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
    )
    
    foreach(lib avcodec avformat swscale avutil)
        find_library(FFMPEG_${lib}_LIB NAMES ${lib}
            PATHS ${ffmpeg_win_SOURCE_DIR}
            PATH_SUFFIXES lib
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
        )
        file(GLOB FFMPEG_${lib}_DLL_LIST "${ffmpeg_win_SOURCE_DIR}/bin/${lib}*.dll")
        if(FFMPEG_${lib}_DLL_LIST)
            list(GET FFMPEG_${lib}_DLL_LIST 0 FFMPEG_${lib}_DLL)
        endif()
        
        if(FFMPEG_${lib}_LIB)
            add_library(FFMPEG::${lib} SHARED IMPORTED)
            set_target_properties(FFMPEG::${lib} PROPERTIES
                IMPORTED_IMPLIB "${FFMPEG_${lib}_LIB}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INC_DIR}"
            )
            if(FFMPEG_${lib}_DLL)
                set_target_properties(FFMPEG::${lib} PROPERTIES
                    IMPORTED_LOCATION "${FFMPEG_${lib}_DLL}"
                )
            endif()
            list(APPEND FFMPEG_LIBRARIES FFMPEG::${lib})
        endif()
    endforeach()
    
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
