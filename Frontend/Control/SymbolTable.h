#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Control {

struct DbgFile {
    unsigned int id = 0;
    std::string name;
    unsigned int size = 0;
};

struct DbgMod {
    unsigned int id = 0;
    std::string name;
    unsigned int fileId = 0;
};

struct DbgType {
    unsigned int id = 0;
    std::string val;
};

struct DbgSym {
    unsigned int id = 0;
    std::string name;
    std::string addrsize;
    int typeId = -1;
    int scopeId = -1;
    int defLineId = -1;
    int val = -1;
    int segId = -1;
};

struct DbgSpan {
    unsigned int id = 0;
    unsigned int segId = 0;
    unsigned int start = 0;
    unsigned int size = 0;
};

struct DbgLine {
    unsigned int id = 0;
    unsigned int fileId = 0;
    unsigned int line = 0;
    std::vector<unsigned int> spanIds;
};

struct DbgScope {
    unsigned int id = 0;
    std::string name;
    std::string type;
    unsigned int size = 0;
    int parentId = -1;
};

struct DbgSeg {
    unsigned int id = 0;
    std::string name;
    unsigned int start = 0;
    unsigned int size = 0;
    std::string addrsize;
    std::string type;
};

class SymbolTable {
public:
    SymbolTable() = default;
    ~SymbolTable() = default;
    SymbolTable(const SymbolTable&) = default;
    SymbolTable& operator=(const SymbolTable&) = default;
    SymbolTable(SymbolTable&&) = default;
    SymbolTable& operator=(SymbolTable&&) = default;

    void Clear();
    bool LoadFromFile(const std::string& path);

    // Get exact symbol mapping at a specific address
    const DbgSym* GetSymbolAtAddress(uint16_t address) const;
    
    // Get line information for a specific address
    const DbgLine* GetLineForAddress(uint16_t address) const;

    const DbgFile* GetFile(unsigned int fileId) const;
    const DbgMod* GetMod(unsigned int modId) const;
    const DbgType* GetType(unsigned int typeId) const;
    const DbgScope* GetScope(unsigned int scopeId) const;
    const DbgSeg* GetSeg(unsigned int segId) const;

    bool IsLoaded() const { return loaded; }
    std::size_t GetSymbolCount() const { return syms.size(); }
    std::size_t GetLabelCount() const { return addrToSymId.size(); }

private:
    bool loaded = false;

    std::unordered_map<unsigned int, DbgFile> files;
    std::unordered_map<unsigned int, DbgMod> mods;
    std::unordered_map<unsigned int, DbgType> types;
    std::unordered_map<unsigned int, DbgSym> syms;
    std::unordered_map<unsigned int, DbgSpan> spans;
    std::unordered_map<unsigned int, DbgLine> lines;
    std::unordered_map<unsigned int, DbgScope> scopes;
    std::unordered_map<unsigned int, DbgSeg> segs;

    // Fast lookups
    std::unordered_map<uint16_t, unsigned int> addrToSymId; 
    std::unordered_map<uint16_t, unsigned int> addrToLineId;
    
    void ProcessRecord(const std::string& type, std::unordered_map<std::string, std::string>& attrs);
};

}  // namespace Control
