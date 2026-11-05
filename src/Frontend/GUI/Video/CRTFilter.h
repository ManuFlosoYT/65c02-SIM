#pragma once

#include <glad/gl.h>
#include "Frontend/GUI/Video/GLPostProcessPass.h"

namespace GUI {

enum class CRTMonitorType : int {
    RGB = 0,
    GreenP1 = 1,  // Phosphor emission peaks: Green P1 (525nm), Amber P3 (590nm), White P4
    AmberP3 = 2,
    WhiteP4 = 3
};

struct CRTParams {
    bool scanlines = false;
    bool interlacing = false;
    bool curvature = false;
    bool chromatic = false;
    bool blur = false;

    bool shadowMask = false;
    bool vignette = false;
    bool cornerRounding = false;
    bool glassGlare = false;

    bool colorBleeding = false;
    bool noise = false;
    bool vsyncJitter = false;
    bool phosphorDecay = false;

    bool bloom = false;
    bool ghosting = false;
    bool halation = false;
    bool moire = false;
    float gamma = 2.75F;

    bool sharpPixels = true;
    bool humBar = false;
    CRTMonitorType monitorType = CRTMonitorType::RGB;
    int nativeWidth = 100;
    int nativeHeight = 75;
    int internalScale = 8;  // 8x8 subpixel scaling: 100x75 VRAM to 800x600 FBO

    float time = 0.0F;
};

class CRTFilter {
   public:
    void Init(int width, int height);
    void Destroy();
    void Reset();
    [[nodiscard]] bool IsInitialized() const { return m_pass.IsInitialized(); }

    GLuint Apply(GLuint inputTex, int width, int height, const CRTParams& params);

   private:
    GLPostProcessPass m_pass;

    GLint m_uTexture = -1;
    GLint m_uPrevFrame = -1;
    GLint m_uNativeSize = -1;
    GLint m_uOutputSize = -1;
    GLint m_uTime = -1;
    GLint m_uScanlines = -1;
    GLint m_uInterlacing = -1;
    GLint m_uCurvature = -1;
    GLint m_uChromatic = -1;
    GLint m_uBlur = -1;
    GLint m_uShadowMask = -1;
    GLint m_uVignette = -1;
    GLint m_uCornerRounding = -1;
    GLint m_uGlassGlare = -1;
    GLint m_uColorBleeding = -1;
    GLint m_uNoise = -1;
    GLint m_uVsyncJitter = -1;
    GLint m_uPhosphorDecay = -1;
    GLint m_uBloom = -1;
    GLint m_uGhosting = -1;
    GLint m_uHalation = -1;
    GLint m_uMoire = -1;
    GLint m_uGamma = -1;
    GLint m_uSharpPixels = -1;
    GLint m_uHumBar = -1;
    GLint m_uMonitorType = -1;
};

}  // namespace GUI