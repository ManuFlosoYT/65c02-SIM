#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifndef TARGET_WASM
#include "Frontend/Compiler/CompilerFrontend.h"
#endif

namespace Control {
struct AppState;
}

namespace GUI {

class IDEBuildDeployer {
public:
    static bool SaveCurrentFileForCompile(Control::AppState& state);

#ifndef TARGET_WASM
    static CompilerFrontend::BuildResult CompileCurrentFile(Control::AppState& state);
    static void HandleCompileAndRun(Control::AppState& state);
    static void HandleCompileAndExport(Control::AppState& state);
#endif

    static bool ExportBinary(Control::AppState& state, const std::string& exportPath);
    static bool ExportApp(Control::AppState& state, const std::string& exportPath);
};

} // namespace GUI
