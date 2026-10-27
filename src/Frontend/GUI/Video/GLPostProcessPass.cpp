#include "Frontend/GUI/Video/GLPostProcessPass.h"

#include <bit>
#include <iostream>
#include <string>
#include <utility>

namespace GUI {

GLPostProcessPass::GLPostProcessPass(GLPostProcessPass&& other) noexcept
    : m_fbo(std::exchange(other.m_fbo, {0, 0})),
      m_outputTex(std::exchange(other.m_outputTex, {0, 0})),
      m_pingPong(std::exchange(other.m_pingPong, 0)),
      m_vao(std::exchange(other.m_vao, 0)),
      m_vbo(std::exchange(other.m_vbo, 0)),
      m_shader(std::exchange(other.m_shader, 0)),
      m_width(std::exchange(other.m_width, 0)),
      m_height(std::exchange(other.m_height, 0)) {}

GLPostProcessPass& GLPostProcessPass::operator=(GLPostProcessPass&& other) noexcept {
    if (this != &other) {
        Destroy();
        m_fbo = std::exchange(other.m_fbo, {0, 0});
        m_outputTex = std::exchange(other.m_outputTex, {0, 0});
        m_pingPong = std::exchange(other.m_pingPong, 0);
        m_vao = std::exchange(other.m_vao, 0);
        m_vbo = std::exchange(other.m_vbo, 0);
        m_shader = std::exchange(other.m_shader, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
    }
    return *this;
}

GLuint GLPostProcessPass::CompileShader(const char* vertSrc, const char* fragSrc) {
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
            glGetShaderInfoLog(shaderId, static_cast<GLsizei>(log.size()), nullptr, log.data());
            std::cerr << "[GLPostProcessPass] Shader compile error: " << log.data() << "\n";
            glDeleteShader(shaderId);
            return 0;
        }
        return shaderId;
    };

    GLuint vert = compile(GL_VERTEX_SHADER, vertSrc);
    if (vert == 0) {
        return 0;
    }

    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);
    if (frag == 0) {
        glDeleteShader(vert);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint isLinked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &isLinked);
    if (isLinked == 0) {
        std::array<char, 512> log{};
        glGetProgramInfoLog(prog, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "[GLPostProcessPass] Shader link error: " << log.data() << "\n";
        glDeleteProgram(prog);
        return 0;
    }

    return prog;
}

void GLPostProcessPass::CreateFBO(int width, int height) {
    m_width = width;
    m_height = height;
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

void GLPostProcessPass::DestroyFBO() {
    if (m_outputTex[0] != 0 || m_outputTex[1] != 0) {
        glDeleteTextures(2, m_outputTex.data());
        m_outputTex = {0, 0};
    }
    if (m_fbo[0] != 0 || m_fbo[1] != 0) {
        glDeleteFramebuffers(2, m_fbo.data());
        m_fbo = {0, 0};
    }
    m_width = 0;
    m_height = 0;
    m_pingPong = 0;
}

bool GLPostProcessPass::Init(int width, int height, const char* vertSrc, const char* fragSrc) {
    Destroy();

    m_shader = CompileShader(vertSrc, fragSrc);
    if (m_shader == 0) {
        return false;
    }

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
    if (posLoc >= 0) {
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    }
    if (uvLoc >= 0) {
        glEnableVertexAttribArray(uvLoc);
        uintptr_t offset = 2 * sizeof(float);
        glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), std::bit_cast<const void*>(offset));
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CreateFBO(width, height);
    return true;
}

void GLPostProcessPass::Destroy() {
    DestroyFBO();
    if (m_shader != 0) {
        glDeleteProgram(m_shader);
        m_shader = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
}

void GLPostProcessPass::Resize(int width, int height) {
    if (width == m_width && height == m_height) {
        return;
    }
    DestroyFBO();
    CreateFBO(width, height);
}

void GLPostProcessPass::SwapPingPong() {
    m_pingPong = 1 - m_pingPong;
}

GLuint GLPostProcessPass::GetCurrentFBO() const {
    return m_fbo[m_pingPong];
}

GLuint GLPostProcessPass::GetCurrentOutputTex() const {
    return m_outputTex[m_pingPong];
}

GLuint GLPostProcessPass::GetPrevOutputTex() const {
    return m_outputTex[1 - m_pingPong];
}

void GLPostProcessPass::RenderQuad() {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

}  // namespace GUI
