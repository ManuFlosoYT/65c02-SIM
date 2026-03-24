file(READ "CMakeLists.txt" CONTENT)
string(REPLACE "VERSION 3.0" "VERSION 3.10" CONTENT "${CONTENT}")
file(WRITE "CMakeLists.txt" "${CONTENT}")
