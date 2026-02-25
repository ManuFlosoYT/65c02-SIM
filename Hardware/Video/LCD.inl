// LCD.inl — Inline implementations for Hardware::LCD

inline std::string Hardware::LCD::GetName() const { return "LCD"; }
inline void Hardware::LCD::SetOutputCallback(std::function<void(char)> cb) {
    onChar = cb;
}
inline const char (&Hardware::LCD::GetScreen() const)[2][16] { return screen; }
inline bool Hardware::LCD::IsInitialized() const { return is_init; }
inline bool Hardware::LCD::IsDisplayOn() const { return display_on; }
inline bool Hardware::LCD::IsCursorOn() const { return cursor_on; }
inline int Hardware::LCD::GetCursorX() const { return cursorX; }
inline int Hardware::LCD::GetCursorY() const { return cursorY; }
