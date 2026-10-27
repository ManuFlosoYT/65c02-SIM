#pragma once

#include <glad/gl.h>
#include <array>

namespace GUI {

class GLPostProcessPass {
public:
    GLPostProcessPass() = default;
    ~GLPostProcessPass() { Destroy(); }

    GLPostProcessPass(const GLPostProcessPass&) = delete;
    GLPostProcessPass& operator=(const GLPostProcessPass&) = delete;
    GLPostProcessPass(GLPostProcessPass&& other) noexcept;
    GLPostProcessPass& operator=(GLPostProcessPass&& other) noexcept;

    static GLuint CompileShader(const char* vertSrc, const char* fragSrc);

    bool Init(int width, int height, const char* vertSrc, const char* fragSrc);
    void Destroy();
    void Resize(int width, int height);
    void SwapPingPong();

    [[nodiscard]] GLuint GetCurrentFBO() const;
    [[nodiscard]] GLuint GetCurrentOutputTex() const;
    [[nodiscard]] GLuint GetPrevOutputTex() const;
    [[nodiscard]] GLuint GetShader() const { return m_shader; }
    [[nodiscard]] bool IsInitialized() const { return m_shader != 0; }

    void RenderQuad();

private:
    void CreateFBO(int width, int height);
    void DestroyFBO();

    std::array<GLuint, 2> m_fbo = {0, 0};
    std::array<GLuint, 2> m_outputTex = {0, 0};
    int m_pingPong = 0;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_shader = 0;

    int m_width = 0;
    int m_height = 0;
};

}  // namespace GUI
