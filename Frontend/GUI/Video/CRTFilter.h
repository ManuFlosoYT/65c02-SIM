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

    // Animated effects time source
    float time = 0.0f;
};

class CRTFilter {
public:
    void Init(int w, int h);
    void Destroy();

    // Renders inputTex through the CRT shader and returns the output texture.
    GLuint Apply(GLuint inputTex, int w, int h, const CRTParams& p);

private:
    GLuint m_fbo = 0;
    GLuint m_outputTex = 0;
    GLuint m_shader = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_w = 0;
    int m_h = 0;

    void CreateFBO(int w, int h);
    void Resize(int w, int h);
    static GLuint CompileShader(const char* vertSrc, const char* fragSrc);
};

}  // namespace GUI
