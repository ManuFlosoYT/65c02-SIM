#ifndef TARGET_WASM
#include "Frontend/GLPboFrameGrabber.h"
#include <glad/gl.h>
#include <cstring>
#include <algorithm>

GLPboFrameGrabber::~GLPboFrameGrabber() {
    Cleanup();
}

void GLPboFrameGrabber::SetupPBOs(std::array<uint32_t, 2>& pbos, int width, int height) {
    glGenBuffers(2, pbos.data());
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos.at(i));
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(width) * height * 4, nullptr, GL_STREAM_READ);
    }
}

void GLPboFrameGrabber::Initialize(int rawW, int rawH, int processedW, int processedH, bool recordRaw, bool recordProcessed) {
    Cleanup();
    rawWidth = rawW;
    rawHeight = rawH;
    processedWidth = processedW;
    processedHeight = processedH;
    shouldRecordRaw = recordRaw;
    shouldRecordProcessed = recordProcessed;

    if (shouldRecordRaw) {
        SetupPBOs(pboRaw, rawWidth, rawHeight);
    }

    if (shouldRecordProcessed) {
        SetupPBOs(pboProcessed, processedWidth, processedHeight);
        pboProcessedCapacity = static_cast<size_t>(processedWidth) * static_cast<size_t>(processedHeight) * 4U;
        pboProcessedW.fill(processedWidth);
        pboProcessedH.fill(processedHeight);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    pboWarmupFrames = 0;
    pboIndex = 0;
}

bool GLPboFrameGrabber::Capture(uint32_t texRaw, uint32_t texProcessed, VideoFrameData& outFrame) {
    int nextIndex = (pboIndex + 1) % 2;

    if (shouldRecordRaw) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboRaw.at(pboIndex));
        glBindTexture(GL_TEXTURE_2D, texRaw);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    if (shouldRecordProcessed) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboProcessed.at(pboIndex));
        glBindTexture(GL_TEXTURE_2D, texProcessed);

        GLint actualProcW = 0;
        GLint actualProcH = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &actualProcW);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &actualProcH);

        if (actualProcW > 0 && actualProcH > 0 && actualProcW < 8192 && actualProcH < 8192) {
            const size_t reqSize = static_cast<size_t>(actualProcW) * static_cast<size_t>(actualProcH) * 4U;
            if (reqSize > pboProcessedCapacity) {
                for (int i = 0; i < 2; ++i) {
                    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboProcessed.at(i));
                    glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(reqSize), nullptr, GL_STREAM_READ);
                }
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pboProcessed.at(pboIndex));
                pboProcessedCapacity = reqSize;
            }
            pboProcessedW.at(pboIndex) = actualProcW;
            pboProcessedH.at(pboIndex) = actualProcH;
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
    }

    bool hasData = false;
    if (pboWarmupFrames >= 1) {
        if (shouldRecordRaw) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboRaw.at(nextIndex));
            const auto* ptrRaw = static_cast<const uint8_t*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
            if (ptrRaw != nullptr) {
                outFrame.rawW = rawWidth;
                outFrame.rawH = rawHeight;
                size_t szRaw = static_cast<size_t>(rawWidth) * static_cast<size_t>(rawHeight) * 4U;
                outFrame.rawPixels.resize(szRaw);
                std::memcpy(outFrame.rawPixels.data(), ptrRaw, szRaw);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                hasData = true;
            }
        }

        if (shouldRecordProcessed) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pboProcessed.at(nextIndex));
            const auto* ptrProc = static_cast<const uint8_t*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
            if (ptrProc != nullptr) {
                const int curW = pboProcessedW.at(nextIndex) > 0 ? pboProcessedW.at(nextIndex) : processedWidth;
                const int curH = pboProcessedH.at(nextIndex) > 0 ? pboProcessedH.at(nextIndex) : processedHeight;
                outFrame.processedW = curW;
                outFrame.processedH = curH;
                size_t szProc = static_cast<size_t>(curW) * static_cast<size_t>(curH) * 4U;
                outFrame.processedPixels.resize(szProc);
                std::memcpy(outFrame.processedPixels.data(), ptrProc, szProc);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                hasData = true;
            }
        }
    } else {
        ++pboWarmupFrames;
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    pboIndex = nextIndex;

    return hasData;
}

void GLPboFrameGrabber::Cleanup() {
    if (pboRaw[0] != 0U) {
        glDeleteBuffers(2, pboRaw.data());
        pboRaw.fill(0U);
    }
    if (pboProcessed[0] != 0U) {
        glDeleteBuffers(2, pboProcessed.data());
        pboProcessed.fill(0U);
        pboProcessedCapacity = 0;
        pboProcessedW.fill(0);
        pboProcessedH.fill(0);
    }
    pboIndex = 0;
    pboWarmupFrames = 0;
}
#endif
