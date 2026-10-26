#pragma once

#include <glad/gl.h>
#include <array>
#include <utility>

namespace Control {

struct GLRenderTargets {
    GLuint vramTexture = 0;
    GLuint profilerTexture = 0;
    GLuint layoutTexture = 0;
    GLuint sidTexture = 0;
    GLuint sidFBO = 0;
    GLuint lastDisplayTex = 0;

    GLRenderTargets() = default;
    ~GLRenderTargets() {
        Destroy();
    }

    GLRenderTargets(const GLRenderTargets&) = delete;
    GLRenderTargets& operator=(const GLRenderTargets&) = delete;

    GLRenderTargets(GLRenderTargets&& other) noexcept {
        MoveFrom(std::move(other));
    }

    GLRenderTargets& operator=(GLRenderTargets&& other) noexcept {
        if (this != &other) {
            Destroy();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    void Destroy() noexcept {
        if (vramTexture != 0U) {
            glDeleteTextures(1, &vramTexture);
            vramTexture = 0;
        }
        if (profilerTexture != 0U) {
            glDeleteTextures(1, &profilerTexture);
            profilerTexture = 0;
        }
        if (layoutTexture != 0U) {
            glDeleteTextures(1, &layoutTexture);
            layoutTexture = 0;
        }
        if (sidTexture != 0U) {
            glDeleteTextures(1, &sidTexture);
            sidTexture = 0;
        }
        if (sidFBO != 0U) {
            glDeleteFramebuffers(1, &sidFBO);
            sidFBO = 0;
        }
        lastDisplayTex = 0;
    }

private:
    void MoveFrom(GLRenderTargets&& other) noexcept {
        vramTexture = other.vramTexture;
        profilerTexture = other.profilerTexture;
        layoutTexture = other.layoutTexture;
        sidTexture = other.sidTexture;
        sidFBO = other.sidFBO;
        lastDisplayTex = other.lastDisplayTex;

        other.vramTexture = 0;
        other.profilerTexture = 0;
        other.layoutTexture = 0;
        other.sidTexture = 0;
        other.sidFBO = 0;
        other.lastDisplayTex = 0;
    }
};

struct RenderState : public GLRenderTargets {
    int lastDisplayW = 0;
    int lastDisplayH = 0;
    std::array<float, 2> sidWindowPos = {0.0F, 0.0F};
    std::array<float, 2> sidWindowSize = {0.0F, 0.0F};
    float uiScale = 1.0F;
};

}  // namespace Control
