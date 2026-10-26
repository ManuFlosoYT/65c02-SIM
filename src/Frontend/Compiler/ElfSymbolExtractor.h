#pragma once
#include <string>
#include "Hardware/Core/SymbolTable.h"

#ifndef TARGET_WASM

class ElfSymbolExtractor {
public:
    static bool LoadELF(Control::SymbolTable& table, const std::string& elfPath);
    static bool GenerateDbgFromMap(const std::string& mapPath, const std::string& dbgPath);
};

#endif
