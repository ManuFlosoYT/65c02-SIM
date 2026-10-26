#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <TextEditor.h>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace Control {
struct AppState;
}

namespace GUI {

struct IDETab {
    uint32_t id = 0;
    std::string title = "Untitled";
    std::string filePath;
    std::string code;
    TextEditor editor;
    bool isReadOnly = false;
    bool requestSelect = false;
};

class IDEDocumentController {
public:
    static std::vector<IDETab>& GetTabs();
    static size_t& GetActiveTabIndex();
    static uint32_t& GetNextTabId();

    static void InitTabs(Control::AppState& state);
    static void UpdateActiveTabLanguageDefinition(Control::AppState& state);
    static void OpenLibInTab(const std::string& lib);
    static void InsertIncludeInActiveTab(Control::AppState& state, const std::string& lib);
    static std::string GetSanitizedTabCode(IDETab& tab);
    static TextEditor::LanguageDefinition GetMOSLanguageDefinition();
    static void SetEditorText(TextEditor& editor, const std::string& text);

    static void OpenDocument(Control::AppState& state, const std::string& filePath, const std::string& content);
    static bool SaveActiveDocument(Control::AppState& state, const std::string& filePath);
    static void CreateNewTab(Control::AppState& state);
    static void CloseTab(size_t index, Control::AppState& state);

#ifndef TARGET_WASM
    static std::vector<CompilerFrontend::Diagnostic>& GetLastDiagnostics();
    static std::optional<size_t> FindTabForDiagnostic(const CompilerFrontend::Diagnostic& diag);
    static void ApplyDiagnosticsToTabs(const std::vector<CompilerFrontend::Diagnostic>& diags);
    static void JumpToDiagnostic(const CompilerFrontend::Diagnostic& diag);
    static void ClearDiagnostics();
#endif
};

} // namespace GUI
