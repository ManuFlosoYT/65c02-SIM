#include "Frontend/GUI/Video/CRTFilter.h"

#include <array>
#include <iostream>

#include "CRTShaders.h"

namespace GUI {

GLuint CRTFilter::CompileShader(const char* vertSrc, const char* fragSrc) {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint shaderId = glCreateShader(type);
        glShaderSource(shaderId, 1, &src, nullptr);
        glCompileShader(shaderId);
        GLint isCompiled = 0;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == 0) {
            std::array<char, 512> log{};
            glGetShaderInfoLog(shaderId, log.size(), nullptr, log.data());
            std::cerr << "[CRTFilter] Shader compile error: " << log.data() << "\n";
        }
        return shaderId;
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

void CRTFilter::CreateFBO(int width, int height) {
    m_w = width;
    m_h = height;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_outputTex);
    glBindTexture(GL_TEXTURE_2D, m_outputTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_outputTex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CRTFilter::Init(int width, int height) {
    m_shader = CompileShader(kCRTVertSrc, kCRTFragSrc);

    // clang-format off
    std::array<float, 24> verts = {
        -1.0F, -1.0F,  0.0F, 0.0F,
         1.0F, -1.0F,  1.0F, 0.0F,
         1.0F,  1.0F,  1.0F, 1.0F,
        -1.0F, -1.0F,  0.0F, 0.0F,
         1.0F,  1.0F,  1.0F, 1.0F,
        -1.0F,  1.0F,  0.0F, 1.0F,
    };
    // clang-format on

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verts.size(), verts.data(), GL_STATIC_DRAW);

    GLint posLoc = glGetAttribLocation(m_shader, "a_pos");
    GLint uvLoc = glGetAttribLocation(m_shader, "a_uv");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(uvLoc);
    uintptr_t offset = 2 * sizeof(float);
    // NOLINTNEXTLINE(performance-no-int-to-ptr, cppcoreguidelines-pro-type-reinterpret-cast)
    glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<const void*>(offset));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CreateFBO(width, height);
}

void CRTFilter::Destroy() {
    if (m_outputTex != 0) {
        glDeleteTextures(1, &m_outputTex);
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
    }
    if (m_shader != 0) {
        glDeleteProgram(m_shader);
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    m_outputTex = 0;
    m_fbo = 0;
    m_shader = 0;
    m_vao = 0;
    m_vbo = 0;
}

void CRTFilter::Resize(int width, int height) {
    if (width == m_w && height == m_h) {
        return;
    }
    glDeleteTextures(1, &m_outputTex);
    glDeleteFramebuffers(1, &m_fbo);
    m_outputTex = 0;
    m_fbo = 0;
    CreateFBO(width, height);
}

GLuint CRTFilter::Apply(GLuint inputTex, int width, int height, const CRTParams& params) {
    Resize(width, height);

    GLint prevFBO = 0;
    std::array<GLint, 4> prevViewport = {0, 0, 0, 0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport.data());

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, width, height);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);

    auto uni1i = [&](const char* n, bool val) { glUniform1i(glGetUniformLocation(m_shader, n), val ? 1 : 0); };

    glUniform1i(glGetUniformLocation(m_shader, "u_texture"), 0);
    glUniform2f(glGetUniformLocation(m_shader, "u_texelSize"), 1.0F / static_cast<float>(width),
                1.0F / static_cast<float>(height));
    glUniform1f(glGetUniformLocation(m_shader, "u_time"), params.time);

    uni1i("u_scanlines", params.scanlines);
    uni1i("u_interlacing", params.interlacing);
    uni1i("u_curvature", params.curvature);
    uni1i("u_chromatic", params.chromatic);
    uni1i("u_blur", params.blur);
    uni1i("u_shadowMask", params.shadowMask);
    uni1i("u_vignette", params.vignette);
    uni1i("u_cornerRounding", params.cornerRounding);
    uni1i("u_glassGlare", params.glassGlare);
    uni1i("u_colorBleeding", params.colorBleeding);
    uni1i("u_noise", params.noise);
    uni1i("u_vsyncJitter", params.vsyncJitter);
    uni1i("u_phosphorDecay", params.phosphorDecay);
    uni1i("u_bloom", params.bloom);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    return m_outputTex;
}

}  // namespace GUI
