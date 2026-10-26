#pragma once

#include "Frontend/CliArgsParser.h"

namespace Frontend {

class HeadlessRunner {
public:
    static int Run(const Args& args, int sidSampleRate = 48000);
};

}  // namespace Frontend
