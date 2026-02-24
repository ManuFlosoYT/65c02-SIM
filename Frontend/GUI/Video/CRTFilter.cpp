#include "Frontend/GUI/Video/CRTFilter.h"

#include <iostream>

#include "Frontend/GUI/Video/CRTShaders.h"

namespace GUI {

GLuint CRTFilter::CompileShader(const char* vertSrc, const char* fragSrc) {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            std::cerr << "[CRTFilter] Shader compile error: " << log << "\n";
        }
        return s;
    };

    GLuint vert = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void CRTFilter::CreateFBO(int w, int h) {
    m_w = w;
    m_h = h;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_outputTex);
    glBindTexture(GL_TEXTURE_2D, m_outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_outputTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CRTFilter::Init(int w, int h) {
    m_shader = CompileShader(kCRTVertSrc, kCRTFragSrc);

    // clang-format off
    float verts[] = {
        -1.f, -1.f,  0.f, 0.f,
         1.f, -1.f,  1.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f, -1.f,  0.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f,  1.f,  0.f, 1.f,
    };
    // clang-format on

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    GLint posLoc = glGetAttribLocation(m_shader, "a_pos");
    GLint uvLoc = glGetAttribLocation(m_shader, "a_uv");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(uvLoc);
    glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CreateFBO(w, h);
}

void CRTFilter::Destroy() {
    if (m_outputTex) glDeleteTextures(1, &m_outputTex);
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    if (m_shader) glDeleteProgram(m_shader);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    m_outputTex = m_fbo = m_shader = m_vao = m_vbo = 0;
}

void CRTFilter::Resize(int w, int h) {
    if (w == m_w && h == m_h) return;
    glDeleteTextures(1, &m_outputTex);
    glDeleteFramebuffers(1, &m_fbo);
    m_outputTex = m_fbo = 0;
    CreateFBO(w, h);
}

GLuint CRTFilter::Apply(GLuint inputTex, int w, int h, const CRTParams& p) {
    Resize(w, h);

    GLint prevFBO, prevViewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);

    auto uni1i = [&](const char* n, bool v) {
        glUniform1i(glGetUniformLocation(m_shader, n), v ? 1 : 0);
    };

    glUniform1i(glGetUniformLocation(m_shader, "u_texture"), 0);
    glUniform2f(glGetUniformLocation(m_shader, "u_texelSize"), 1.f / w,
                1.f / h);
    glUniform1f(glGetUniformLocation(m_shader, "u_time"), p.time);

    uni1i("u_scanlines", p.scanlines);
    uni1i("u_interlacing", p.interlacing);
    uni1i("u_curvature", p.curvature);
    uni1i("u_chromatic", p.chromatic);
    uni1i("u_blur", p.blur);
    uni1i("u_shadowMask", p.shadowMask);
    uni1i("u_vignette", p.vignette);
    uni1i("u_cornerRounding", p.cornerRounding);
    uni1i("u_glassGlare", p.glassGlare);
    uni1i("u_colorBleeding", p.colorBleeding);
    uni1i("u_noise", p.noise);
    uni1i("u_vsyncJitter", p.vsyncJitter);
    uni1i("u_phosphorDecay", p.phosphorDecay);
    uni1i("u_bloom", p.bloom);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
               prevViewport[3]);

    return m_outputTex;
}

}  // namespace GUI
