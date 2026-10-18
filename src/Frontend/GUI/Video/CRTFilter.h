#pragma once

#include <glad/gl.h>

namespace GUI {

struct CRTParams {
    // Essentials
    bool scanlines = false;
    bool interlacing = false;
    bool curvature = false;
    bool chromatic = false;
    bool blur = false;

    // Screen Physicality
    bool shadowMask = false;
    bool vignette = false;
    bool cornerRounding = false;
    bool glassGlare = false;

    // Signal & Analog Imperfections
    bool colorBleeding = false;
    bool noise = false;
    bool vsyncJitter = false;
    bool phosphorDecay = false;

    // Lighting
    bool bloom = false;
    bool ghosting = false;
    bool halation = false;
    bool moire = false;
    float gamma = 2.75F;

    // Animated effects time source
    float time = 0.0F;
};

class CRTFilter {
   public:
    void Init(int width, int height);
    void Destroy();

    // Renders inputTex through the CRT shader and returns the output texture.
    GLuint Apply(GLuint inputTex, int width, int height, const CRTParams& params);

   private:
    GLuint m_fbo = 0;
    GLuint m_outputTex = 0;
    GLuint m_shader = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_w = 0;
    int m_h = 0;

    GLint m_uTexture = -1;
    GLint m_uTexelSize = -1;
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

    void CreateFBO(int width, int height);
    void Resize(int width, int height);
    static GLuint CompileShader(const char* vertSrc, const char* fragSrc);
};

}  // namespace GUI
