#include "Frontend/GUI/Video/CRTFilter.h"

#include <algorithm>
#include <array>

#include "CRTShaders.h"

namespace GUI {

void CRTFilter::Init(int width, int height) {
    int initW = (width <= 200) ? width * 8 : width;
    int initH = (height <= 150) ? height * 8 : height;

    if (!m_pass.Init(initW, initH, kCRTVertSrc, kCRTFragSrc)) {
        return;
    }

    GLuint shader = m_pass.GetShader();
    m_uTexture = glGetUniformLocation(shader, "u_texture");
    m_uPrevFrame = glGetUniformLocation(shader, "u_prevFrame");
    m_uNativeSize = glGetUniformLocation(shader, "u_nativeSize");
    m_uOutputSize = glGetUniformLocation(shader, "u_outputSize");
    m_uTime = glGetUniformLocation(shader, "u_time");
    m_uScanlines = glGetUniformLocation(shader, "u_scanlines");
    m_uInterlacing = glGetUniformLocation(shader, "u_interlacing");
    m_uCurvature = glGetUniformLocation(shader, "u_curvature");
    m_uChromatic = glGetUniformLocation(shader, "u_chromatic");
    m_uBlur = glGetUniformLocation(shader, "u_blur");
    m_uShadowMask = glGetUniformLocation(shader, "u_shadowMask");
    m_uVignette = glGetUniformLocation(shader, "u_vignette");
    m_uCornerRounding = glGetUniformLocation(shader, "u_cornerRounding");
    m_uGlassGlare = glGetUniformLocation(shader, "u_glassGlare");
    m_uColorBleeding = glGetUniformLocation(shader, "u_colorBleeding");
    m_uNoise = glGetUniformLocation(shader, "u_noise");
    m_uVsyncJitter = glGetUniformLocation(shader, "u_vsyncJitter");
    m_uPhosphorDecay = glGetUniformLocation(shader, "u_phosphorDecay");
    m_uBloom = glGetUniformLocation(shader, "u_bloom");
    m_uGhosting = glGetUniformLocation(shader, "u_ghosting");
    m_uHalation = glGetUniformLocation(shader, "u_halation");
    m_uMoire = glGetUniformLocation(shader, "u_moire");
    m_uGamma = glGetUniformLocation(shader, "u_gamma");
    m_uSharpPixels = glGetUniformLocation(shader, "u_sharpPixels");
    m_uHumBar = glGetUniformLocation(shader, "u_humBar");
    m_uMonitorType = glGetUniformLocation(shader, "u_monitorType");
}

void CRTFilter::Destroy() {
    m_pass.Destroy();
}

GLuint CRTFilter::Apply(GLuint inputTex, int width, int height, const CRTParams& params) {
    if (!m_pass.IsInitialized()) {
        return inputTex;
    }

    int scale = std::max(params.internalScale, 1);
    int outW = (width <= params.nativeWidth * 2) ? params.nativeWidth * scale : width;
    int outH = (height <= params.nativeHeight * 2) ? params.nativeHeight * scale : height;

    m_pass.Resize(outW, outH);

    GLint prevFBO = 0;
    std::array<GLint, 4> prevViewport = {0, 0, 0, 0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    glGetIntegerv(GL_VIEWPORT, prevViewport.data());

    glBindFramebuffer(GL_FRAMEBUFFER, m_pass.GetCurrentFBO());
    glViewport(0, 0, outW, outH);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_pass.GetShader());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);

    GLint prevMinFilter = GL_NEAREST;
    GLint prevMagFilter = GL_NEAREST;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &prevMinFilter);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &prevMagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(m_uTexture, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pass.GetPrevOutputTex());
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

    m_pass.RenderQuad();

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, prevMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, prevMagFilter);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    GLuint outTex = m_pass.GetCurrentOutputTex();
    m_pass.SwapPingPong();
    return outTex;
}

}  // namespace GUI
