# Patch httplib to disable problematic non-blocking networking on Windows
file(READ "CMakeLists.txt" CONTENT)
string(REPLACE "target_compile_definitions(httplib INTERFACE CPPHTTPLIB_USE_NON_BLOCKING_GETADDRINFO)" "# Disabled by 65c02-SIM patch" CONTENT "${CONTENT}")
file(WRITE "CMakeLists.txt" "${CONTENT}")
