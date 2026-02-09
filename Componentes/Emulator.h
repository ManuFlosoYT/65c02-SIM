#include <deque>
#include <functional>
#include <string>

#include "ACIA.h"
#include "CPU.h"
#include "LCD.h"
#include "Mem.h"

class Emulator {
public:
    Emulator();
    ~Emulator() = default;

    bool Init(const std::string& binPath, std::string& errorMsg);
    int Step();
    void InjectKey(char c);

    void SetOutputCallback(std::function<void(char)> cb);
    void SetLCDOutputCallback(std::function<void(char)> cb) {
        lcd.SetOutputCallback(cb);
    }

    const char (&GetLCDScreen() const)[2][16] { return lcd.GetScreen(); }
    const CPU& GetCPU() const { return cpu; }
    const Mem& GetMem() const { return mem; }

    void PrintState();

private:
    // Componentes
    Mem mem;
    CPU cpu;
    LCD lcd;
    ACIA acia;

    // Buffer de entrada
    std::deque<char> inputBuffer;
    int baudDelay = 0;
};
