# Patch PocketPy source for case-sensitive filesystems on Linux
file(GLOB_RECURSE SOURCES "src/*.h" "src/*.c" "src/*.cpp" "Unity/*.h" "Unity/*.c" "Unity/*.cpp")

foreach(file ${SOURCES})
    file(READ "${file}" CONTENT)
    string(REPLACE "Include <WinSock2.h>" "Include <winsock2.h>" CONTENT "${CONTENT}")
    string(REPLACE "include <WinSock2.h>" "include <winsock2.h>" CONTENT "${CONTENT}")
    string(REPLACE "Include <WS2tcpip.h>" "Include <ws2tcpip.h>" CONTENT "${CONTENT}")
    string(REPLACE "include <WS2tcpip.h>" "include <ws2tcpip.h>" CONTENT "${CONTENT}")
    file(WRITE "${file}" "${CONTENT}")
endforeach()
