#pragma once

#include <array>
#include <functional>
#include <iostream>

#include "Hardware/Core/IBusDevice.h"

namespace Hardware {

class LCD : public IBusDevice {
   public:
    LCD();
    void Reset() override;

    // IBusDevice implementation
    Byte Read(Word address) override;
    void Write(Word address, Byte data) override;
    [[nodiscard]] std::string GetName() const override;

    void Update(Byte portBVal);

    bool SaveState(std::ostream& out) const override;
    bool LoadState(std::istream& inputStream) override;

    void SetOutputCallback(std::function<void(char)> callback);

    [[nodiscard]] const std::array<std::array<char, 16>, 2>& GetScreen() const;

    [[nodiscard]] bool IsInitialized() const;

    [[nodiscard]] bool IsDisplayOn() const;
    [[nodiscard]] bool IsCursorOn() const;
    [[nodiscard]] int GetCursorX() const;
    [[nodiscard]] int GetCursorY() const;

   private:
    Byte PORTB_DATA;
    Byte DDRB_DATA;

    bool four_bit_mode = false;
    bool waiting_low_nibble = false;
    Byte current_high_nibble = 0;
    Byte last_portb = 0;

    bool is_init = false;

    // Display State
    std::array<std::array<char, 16>, 2> screen;
    int cursorX = 0;
    int cursorY = 0;

    bool display_on = true;
    bool cursor_on = false;
    bool cursor_increment = true;

    std::function<void(char)> onChar;

    void WriteCharToScreen(char character);
    void HandleCommand(Byte cmd);
};

}  // namespace Hardware

inline std::string Hardware::LCD::GetName() const { return "LCD"; }
inline void Hardware::LCD::SetOutputCallback(std::function<void(char)> callback) { onChar = std::move(callback); }
inline const std::array<std::array<char, 16>, 2>& Hardware::LCD::GetScreen() const { return screen; }
inline bool Hardware::LCD::IsInitialized() const { return is_init; }
inline bool Hardware::LCD::IsDisplayOn() const { return display_on; }
inline bool Hardware::LCD::IsCursorOn() const { return cursor_on; }
inline int Hardware::LCD::GetCursorX() const { return cursorX; }
inline int Hardware::LCD::GetCursorY() const { return cursorY; }
