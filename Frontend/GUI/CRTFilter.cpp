#include "Frontend/GUI/CRTFilter.h"

#include <iostream>

namespace GUI {

// ---------------------------------------------------------------------------
// GLSL 130 shaders
// ---------------------------------------------------------------------------

static const char* kVertSrc = R"(
#version 130
in vec2 a_pos;
in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)";

static const char* kFragSrc = R"(
#version 130
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_texture;
uniform bool u_scanlines;
uniform bool u_curvature;
uniform bool u_chromatic;

vec2 curveUV(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(5.5, 4.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;
}

void main() {
    vec2 uv = v_uv;

    if (u_curvature) {
        uv = curveUV(uv);
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    vec4 color;
    if (u_chromatic) {
        float shift = 0.004;
        float r = texture(u_texture, uv + vec2(shift, 0.0)).r;
        float g = texture(u_texture, uv).g;
        float b = texture(u_texture, uv - vec2(shift, 0.0)).b;
        color = vec4(r, g, b, 1.0);
    } else {
        color = texture(u_texture, uv);
    }

    if (u_scanlines) {
        float line = mod(gl_FragCoord.y, 2.0);
        if (line < 1.0) color.rgb *= 0.6;
    }

    fragColor = color;
}
)";

// ---------------------------------------------------------------------------

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
    m_shader = CompileShader(kVertSrc, kFragSrc);

    // Fullscreen quad: positions + UVs
    // clang-format off
    float verts[] = {
        // pos        // uv
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

GLuint CRTFilter::Apply(GLuint inputTex, int w, int h, bool scanlines,
                        bool curvature, bool chromatic) {
    Resize(w, h);

    // Save previous GL state
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
    glUniform1i(glGetUniformLocation(m_shader, "u_texture"), 0);
    glUniform1i(glGetUniformLocation(m_shader, "u_scanlines"),
                scanlines ? 1 : 0);
    glUniform1i(glGetUniformLocation(m_shader, "u_curvature"),
                curvature ? 1 : 0);
    glUniform1i(glGetUniformLocation(m_shader, "u_chromatic"),
                chromatic ? 1 : 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore previous state
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
               prevViewport[3]);

    return m_outputTex;
}

}  // namespace GUI
