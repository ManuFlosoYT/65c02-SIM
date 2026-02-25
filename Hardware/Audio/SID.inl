// SID.inl — Inline implementations for Hardware::SID

inline std::string Hardware::SID::GetName() const { return "SID"; }
inline bool Hardware::SID::IsSoundEnabled() const { return soundEnabled; }
inline const Hardware::Oscillator& Hardware::SID::GetVoice(int index) const {
    return voices[index % MAX_SID_VOICES];
}
