// ACIA.inl — Inline implementations for Hardware::ACIA

inline std::string Hardware::ACIA::GetName() const { return "ACIA"; }
inline void Hardware::ACIA::SetOutputCallback(std::function<void(char)> cb) {
    outputCallback = cb;
}
