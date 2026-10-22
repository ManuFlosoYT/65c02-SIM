#include "Frontend/GUI/Video/CRTFilter.h"

#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <string>

#include "CRTShaders.h"

namespace GUI {

GLuint CRTFilter::CompileShader(const char* vertSrc, const char* fragSrc) {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        const char* shaderSourcePtr = nullptr;
#ifdef TARGET_WASM
        std::string finalSrc = "#version 300 es\nprecision highp float;\n";
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

    GLint isLinked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &isLinked);
    if (isLinked == 0) {
        std::array<char, 512> log{};
        glGetProgramInfoLog(prog, log.size(), nullptr, log.data());
        std::cerr << "[CRTFilter] Shader link error: " << log.data() << "\n";
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

void CRTFilter::CreateFBO(int width, int height) {
    m_w = width;
    m_h = height;
    m_pingPong = 0;

    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

    glGenFramebuffers(2, m_fbo.data());
    glGenTextures(2, m_outputTex.data());

    for (size_t i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[i]);

        glBindTexture(GL_TEXTURE_2D, m_outputTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_outputTex[i], 0);

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CRTFilter::Init(int width, int height) {
    m_shader = CompileShader(kCRTVertSrc, kCRTFragSrc);

    m_uTexture = glGetUniformLocation(m_shader, "u_texture");
    m_uPrevFrame = glGetUniformLocation(m_shader, "u_prevFrame");
    m_uNativeSize = glGetUniformLocation(m_shader, "u_nativeSize");
    m_uOutputSize = glGetUniformLocation(m_shader, "u_outputSize");
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
    m_uSharpPixels = glGetUniformLocation(m_shader, "u_sharpPixels");
    m_uHumBar = glGetUniformLocation(m_shader, "u_humBar");
    m_uMonitorType = glGetUniformLocation(m_shader, "u_monitorType");

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

    // If we initialize with 100x75, we reserve directly 800x600 for the CRT effects
    int initW = (width <= 200) ? width * 8 : width;
    int initH = (height <= 150) ? height * 8 : height;
    CreateFBO(initW, initH);
}

void CRTFilter::Destroy() {
    if (m_outputTex[0] != 0 || m_outputTex[1] != 0) {
        glDeleteTextures(2, m_outputTex.data());
    }
    if (m_fbo[0] != 0 || m_fbo[1] != 0) {
        glDeleteFramebuffers(2, m_fbo.data());
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
    m_outputTex = {0, 0};
    m_fbo = {0, 0};
    m_shader = 0;
    m_vao = 0;
    m_vbo = 0;
}

void CRTFilter::Resize(int width, int height) {
    if (width == m_w && height == m_h) {
        return;
    }
    glDeleteTextures(2, m_outputTex.data());
    glDeleteFramebuffers(2, m_fbo.data());
    m_outputTex = {0, 0};
    m_fbo = {0, 0};
    CreateFBO(width, height);
}

GLuint CRTFilter::Apply(GLuint inputTex, int width, int height, const CRTParams& params) {
    // If we receive the native resolution (100x75), we scale the FBO so that
    // the scanlines, shadow mask and curvature have real subpixels (800x600).
    int scale = std::max(params.internalScale, 1);
    int outW = (width <= params.nativeWidth * 2) ? params.nativeWidth * scale : width;
    int outH = (height <= params.nativeHeight * 2) ? params.nativeHeight * scale : height;

    Resize(outW, outH);

    const int writeIdx = m_pingPong;
    const int readIdx = 1 - m_pingPong;

    GLint prevFBO = 0;
    std::array<GLint, 4> prevViewport = {0, 0, 0, 0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport.data());

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo[writeIdx]);
    glViewport(0, 0, outW, outH);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);

    // We save the original filter of inputTex and force GL_LINEAR during the pass
    // so that sampleVRAM() can profile smooth sub-texel edges over the 100x75 pixels.
    GLint prevMinFilter = GL_NEAREST;
    GLint prevMagFilter = GL_NEAREST;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &prevMinFilter);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &prevMagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(m_uTexture, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_outputTex[readIdx]);
    glUniform1i(m_uPrevFrame, 1);

    glUniform2f(m_uNativeSize, static_cast<float>(params.nativeWidth),
                static_cast<float>(params.nativeHeight));
    glUniform2f(m_uOutputSize, static_cast<float>(outW),
                static_cast<float>(outH));
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

    glUniform1i(m_uSharpPixels, params.sharpPixels ? 1 : 0);
    glUniform1i(m_uHumBar, params.humBar ? 1 : 0);
    glUniform1i(m_uMonitorType, static_cast<int>(params.monitorType));

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, prevMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, prevMagFilter);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    m_pingPong = readIdx;
    return m_outputTex[writeIdx];
}

}  // namespace GUI