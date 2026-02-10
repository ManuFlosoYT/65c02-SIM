#include <deque>
#include <functional>
#include <mutex>
#include <string>

#include "ACIA.h"
#include "CPU.h"
#include "GPU.h"
#include "LCD.h"
#include "Mem.h"
#include "SID.h"

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
    GPU& GetGPU() { return gpu; }
    void SetGPUEnabled(bool enabled) { gpuEnabled = enabled; }
    bool IsGPUEnabled() const { return gpuEnabled; }

    SID& GetSID() { return sid; }

    void PrintState();

private:
    // Componentes
    Mem mem;
    CPU cpu;
    LCD lcd;
    ACIA acia;
    GPU gpu;
    SID sid;

    // Buffer de entrada
    std::deque<char> inputBuffer;
    std::mutex bufferMutex;
    int baudDelay = 0;
    bool gpuEnabled = false;
};
