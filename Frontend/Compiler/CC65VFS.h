#pragma once
#include <string>
#include <vector>
#include <cstdint>

#ifndef EMSCRIPTEN

class CC65VFS {
public:
    static void Initialize();
    static void Cleanup();
    static std::string GetTempDir();
};

#endif
