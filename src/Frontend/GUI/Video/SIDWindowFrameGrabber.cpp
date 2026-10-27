#include "Frontend/GUI/Video/SIDWindowFrameGrabber.h"
#include <glad/gl.h>
#include <imgui.h>

namespace GUI {

void SIDWindowFrameGrabber::CaptureSIDWindow(Control::AppState& state) {
    if (state.render.sidFBO == 0) {
        glGenTextures(1, &state.render.sidTexture);
        glBindTexture(GL_TEXTURE_2D, state.render.sidTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glGenFramebuffers(1, &state.render.sidFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, state.render.sidFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.render.sidTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    const ImGuiIO& imguiIO = ImGui::GetIO();
    int screenHeightPixels = static_cast<int>(imguiIO.DisplaySize.y * imguiIO.DisplayFramebufferScale.y);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state.render.sidFBO);

    float scaleX = imguiIO.DisplayFramebufferScale.x;
    float scaleY = imguiIO.DisplayFramebufferScale.y;

    float windowX = state.render.sidWindowPos[0];
    float windowY = state.render.sidWindowPos[1];
    float windowW = state.render.sidWindowSize[0];
    float windowH = state.render.sidWindowSize[1];

    if (windowW <= 0 || windowH <= 0) {
        return;
    }

    int srcX0 = static_cast<int>(windowX * scaleX);
    int srcY0 = screenHeightPixels - static_cast<int>((windowY + windowH) * scaleY);
    int srcX1 = static_cast<int>((windowX + windowW) * scaleX);
    int srcY1 = screenHeightPixels - static_cast<int>(windowY * scaleY);

    float targetW = 1280.0F;
    float targetH = 720.0F;
    float winAR = windowW / windowH;
    float targetAR = targetW / targetH;

    int drawW = 0;
    int drawH = 0;
    int offsetX = 0;
    int offsetY = 0;

    if (winAR > targetAR) {
        drawW = 1280;
        drawH = static_cast<int>(1280.0F / winAR);
        offsetY = (720 - drawH) / 2;
    } else {
        drawH = 720;
        drawW = static_cast<int>(720.0F * winAR);
        offsetX = (1280 - drawW) / 2;
    }

    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
                      offsetX, offsetY + drawH, offsetX + drawW, offsetY,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CaptureSIDWindow(Control::AppState& state) {
    SIDWindowFrameGrabber::CaptureSIDWindow(state);
}

}  // namespace GUI
