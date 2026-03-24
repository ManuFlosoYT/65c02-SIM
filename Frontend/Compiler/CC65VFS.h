#pragma once
#include <string>

#ifndef TARGET_WASM

class CC65VFS {
public:
    static void Initialize();
    static void Cleanup();
    static std::string GetTempDir();
};

#endif
