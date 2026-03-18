file(READ "CMakeLists.txt" CONTENT)
string(REPLACE "VERSION 3.1..3.18" "VERSION 3.10..3.18" CONTENT "${CONTENT}")
file(WRITE "CMakeLists.txt" "${CONTENT}")
