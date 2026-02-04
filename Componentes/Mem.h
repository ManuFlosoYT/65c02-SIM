#ifndef SIM_65C02_MEM_H
#define SIM_65C02_MEM_H

#include <cstdint>
#include <functional>
#include <map>

using Byte = uint8_t;
using Word = uint16_t;

#define ACIA_DATA 0x5000
#define ACIA_STATUS 0x5001
#define ACIA_CMD 0x5002
#define ACIA_CTRL 0x5003

#define PORTB 0x6000
#define DDRB 0x6002

#define PORTA 0x6001
#define DDRA 0x6003

class Mem {
public:
    constexpr static Word MAX_MEM = 0xFFFF;              // 64KB de memoria
    constexpr static Word ROM_SIZE = (MAX_MEM / 2) + 1;  // 32KB de ROM

    Mem() { Init(); }
    Byte memoria[MAX_MEM]{};
    void Init();

    Byte operator[](Word dir) const { return memoria[dir]; }

    using WriteHook = std::function<void(Word, Byte)>;
    void SetWriteHook(Word address, WriteHook hook);

    using ReadHook = std::function<Byte(Word)>;
    void SetReadHook(Word address, ReadHook hook);

    void Write(Word dir, Byte val);
    void WriteWord(Word dato, Word dir);
    Byte Read(Word dir);

private:
    std::map<Word, WriteHook> writeHooks;
    std::map<Word, ReadHook> readHooks;
};

#endif  // SIM_65C02_MEM_H