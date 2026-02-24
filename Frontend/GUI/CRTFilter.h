#pragma once

#include <glad/gl.h>

namespace GUI {

class CRTFilter {
public:
    void Init(int w, int h);
    void Destroy();
    void Resize(int w, int h);

    // Renders inputTex through the CRT shader and returns the output texture.
    // If all effects are disabled this still performs the blit but with a
    // pass-through shader — callers should skip the call entirely in that case.
    GLuint Apply(GLuint inputTex, int w, int h, 
        bool scanlines, bool curvature, bool chromatic);

private:
    GLuint m_fbo = 0;
    GLuint m_outputTex = 0;
    GLuint m_shader = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_w = 0;
    int m_h = 0;

    void CreateFBO(int w, int h);
    static GLuint CompileShader(const char* vertSrc, const char* fragSrc);
};

}  // namespace GUI
