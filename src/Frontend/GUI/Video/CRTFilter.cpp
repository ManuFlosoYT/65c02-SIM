#include "Frontend/GUI/Video/CRTFilter.h"

#include <bit>
#include <array>
#include <iostream>

#include "CRTShaders.h"

namespace GUI {

GLuint CRTFilter::CompileShader(const char* vertSrc, const char* fragSrc) {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        const char* shaderSourcePtr = nullptr;
#ifdef TARGET_WASM
        std::string finalSrc;
        if (type == GL_VERTEX_SHADER) {
            finalSrc = "#version 300 es\n";
        } else {
            finalSrc = "#version 300 es\nprecision mediump float;\n";
        }
        std::string s = src;
        size_t verPos = s.find("#version");
        if (verPos != std::string::npos) {
            size_t lineEnd = s.find('\n', verPos);
            if (lineEnd != std::string::npos) {
                s.erase(verPos, lineEnd - verPos + 1);
            }
        }
        finalSrc += s;
        shaderSourcePtr = finalSrc.c_str();
#else
        shaderSourcePtr = src;
#endif
        GLuint shaderId = glCreateShader(type);
        glShaderSource(shaderId, 1, &shaderSourcePtr, nullptr);
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

    m_uTexture = glGetUniformLocation(m_shader, "u_texture");
    m_uTexelSize = glGetUniformLocation(m_shader, "u_texelSize");
    m_uTime = glGetUniformLocation(m_shader, "u_time");
    m_uScanlines = glGetUniformLocation(m_shader, "u_scanlines");
    m_uInterlacing = glGetUniformLocation(m_shader, "u_interlacing");
    m_uCurvature = glGetUniformLocation(m_shader, "u_curvature");
    m_uChromatic = glGetUniformLocation(m_shader, "u_chromatic");
    m_uBlur = glGetUniformLocation(m_shader, "u_blur");
    m_uShadowMask = glGetUniformLocation(m_shader, "u_shadowMask");
    m_uVignette = glGetUniformLocation(m_shader, "u_vignette");
    m_uCornerRounding = glGetUniformLocation(m_shader, "u_cornerRounding");
    m_uGlassGlare = glGetUniformLocation(m_shader, "u_glassGlare");
    m_uColorBleeding = glGetUniformLocation(m_shader, "u_colorBleeding");
    m_uNoise = glGetUniformLocation(m_shader, "u_noise");
    m_uVsyncJitter = glGetUniformLocation(m_shader, "u_vsyncJitter");
    m_uPhosphorDecay = glGetUniformLocation(m_shader, "u_phosphorDecay");
    m_uBloom = glGetUniformLocation(m_shader, "u_bloom");
    m_uGhosting = glGetUniformLocation(m_shader, "u_ghosting");
    m_uHalation = glGetUniformLocation(m_shader, "u_halation");
    m_uMoire = glGetUniformLocation(m_shader, "u_moire");
    m_uGamma = glGetUniformLocation(m_shader, "u_gamma");

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
    glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), std::bit_cast<const void*>(offset));

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

    glUniform1i(m_uTexture, 0);
    glUniform2f(m_uTexelSize, 1.0F / static_cast<float>(width),
                1.0F / static_cast<float>(height));
    glUniform1f(m_uTime, params.time);

    glUniform1i(m_uScanlines, params.scanlines ? 1 : 0);
    glUniform1i(m_uInterlacing, params.interlacing ? 1 : 0);
    glUniform1i(m_uCurvature, params.curvature ? 1 : 0);
    glUniform1i(m_uChromatic, params.chromatic ? 1 : 0);
    glUniform1i(m_uBlur, params.blur ? 1 : 0);
    glUniform1i(m_uShadowMask, params.shadowMask ? 1 : 0);
    glUniform1i(m_uVignette, params.vignette ? 1 : 0);
    glUniform1i(m_uCornerRounding, params.cornerRounding ? 1 : 0);
    glUniform1i(m_uGlassGlare, params.glassGlare ? 1 : 0);
    glUniform1i(m_uColorBleeding, params.colorBleeding ? 1 : 0);
    glUniform1i(m_uNoise, params.noise ? 1 : 0);
    glUniform1i(m_uVsyncJitter, params.vsyncJitter ? 1 : 0);
    glUniform1i(m_uPhosphorDecay, params.phosphorDecay ? 1 : 0);
    glUniform1i(m_uBloom, params.bloom ? 1 : 0);
    glUniform1i(m_uGhosting, params.ghosting ? 1 : 0);
    glUniform1i(m_uHalation, params.halation ? 1 : 0);
    glUniform1i(m_uMoire, params.moire ? 1 : 0);
    glUniform1f(m_uGamma, params.gamma);

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
