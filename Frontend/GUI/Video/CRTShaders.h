#pragma once

// Vertex shader
static const char* kCRTVertSrc = R"(
#version 130
in vec2 a_pos;
in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)";

// Fragment shader
static const char* kCRTFragSrc = R"(
#version 130
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_texture;
uniform vec2  u_texelSize;
uniform float u_time;

// Essentials
uniform bool u_scanlines;
uniform bool u_curvature;
uniform bool u_chromatic;
uniform bool u_blur;

// Screen Physicality
uniform bool u_shadowMask;
uniform bool u_vignette;
uniform bool u_cornerRounding;
uniform bool u_glassGlare;

// Signal & Analog
uniform bool u_colorBleeding;
uniform bool u_noise;
uniform bool u_vsyncJitter;
uniform bool u_phosphorDecay;

// Lighting
uniform bool u_bloom;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 curveUV(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(5.5, 4.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;
}

// Rounded-rectangle SDF: returns >0 outside corners
float roundedCornerDist(vec2 uv, float radius) {
    vec2 c = abs(uv * 2.0 - 1.0) - (1.0 - radius);
    return length(max(c, 0.0)) - radius;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

void main() {
    vec2 uv = v_uv;

    // --- UV warping phase ---

    if (u_curvature) {
        uv = curveUV(uv);
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    if (u_cornerRounding) {
        float r = 0.07;
        if (roundedCornerDist(uv, r) > 0.0) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    if (u_vsyncJitter) {
        float band  = float(int(uv.y * (1.0 / u_texelSize.y) * 0.03));
        float drift = (hash(vec2(band, u_time * 0.1)) - 0.5) * u_texelSize.x * 0.4;
        uv.x += drift;
    }

    // --- Base color sample phase ---

    vec4 color;

    if (u_blur) {
        // 3x3 Gaussian - phosphor glow/softness
        vec4 sum = vec4(0.0);
        sum += texture(u_texture, uv + vec2(-1.0, -1.0) * u_texelSize) * 0.0625;
        sum += texture(u_texture, uv + vec2( 0.0, -1.0) * u_texelSize) * 0.125;
        sum += texture(u_texture, uv + vec2( 1.0, -1.0) * u_texelSize) * 0.0625;
        sum += texture(u_texture, uv + vec2(-1.0,  0.0) * u_texelSize) * 0.125;
        sum += texture(u_texture, uv)                                   * 0.25;
        sum += texture(u_texture, uv + vec2( 1.0,  0.0) * u_texelSize) * 0.125;
        sum += texture(u_texture, uv + vec2(-1.0,  1.0) * u_texelSize) * 0.0625;
        sum += texture(u_texture, uv + vec2( 0.0,  1.0) * u_texelSize) * 0.125;
        sum += texture(u_texture, uv + vec2( 1.0,  1.0) * u_texelSize) * 0.0625;
        if (u_chromatic) {
            float s = 0.004;
            float r = texture(u_texture, uv + vec2( s, 0.0)).r;
            float b = texture(u_texture, uv + vec2(-s, 0.0)).b;
            color = vec4(r * 0.5 + sum.r * 0.5, sum.g, b * 0.5 + sum.b * 0.5, 1.0);
        } else {
            color = sum;
        }
    } else if (u_chromatic) {
        float s = 0.004;
        float r = texture(u_texture, uv + vec2( s, 0.0)).r;
        float g = texture(u_texture, uv).g;
        float b = texture(u_texture, uv - vec2( s, 0.0)).b;
        color = vec4(r, g, b, 1.0);
    } else {
        color = texture(u_texture, uv);
    }

    // --- Post-sample color effects ---

    if (u_colorBleeding) {
        float d = u_texelSize.x * 3.0;
        color.r = mix(color.r, texture(u_texture, uv + vec2( d, 0.0)).r, 0.3);
        color.b = mix(color.b, texture(u_texture, uv - vec2( d, 0.0)).b, 0.3);
    }

    if (u_phosphorDecay) {
        vec4 t1 = texture(u_texture, uv + vec2(0.0, -u_texelSize.y * 2.0));
        vec4 t2 = texture(u_texture, uv + vec2(0.0, -u_texelSize.y * 5.0));
        color.rgb = max(color.rgb, t1.rgb * 0.2 + t2.rgb * 0.08);
    }

    if (u_bloom) {
        vec4 bsum = vec4(0.0);
        float br = 3.0;
        for (int dx = -2; dx <= 2; dx++) {
            for (int dy = -2; dy <= 2; dy++) {
                vec4 s = texture(u_texture, uv + vec2(float(dx), float(dy)) * u_texelSize * br);
                float luma = dot(s.rgb, vec3(0.2126, 0.7152, 0.0722));
                bsum += s * max(luma - 0.6, 0.0);
            }
        }
        color.rgb += (bsum.rgb / 25.0) * 0.35;
    }

    // --- Screen artifact effects ---

    if (u_shadowMask) {
        int px = int(mod(gl_FragCoord.x, 3.0));
        vec3 mask = vec3(1.0);
        if      (px == 0) { mask.g *= 0.7; mask.b *= 0.7; }
        else if (px == 1) { mask.r *= 0.7; mask.b *= 0.7; }
        else              { mask.r *= 0.7; mask.g *= 0.7; }
        color.rgb *= mask;
    }

    if (u_scanlines) {
        float line = mod(gl_FragCoord.y, 2.0);
        if (line < 1.0) color.rgb *= 0.6;
    }

    if (u_noise) {
        float n = (hash(uv + vec2(u_time * 0.07, u_time * 0.13)) - 0.5) * 0.06;
        color.rgb = clamp(color.rgb + n, 0.0, 1.0);
    }

    // --- Screen geometry / lighting ---

    if (u_vignette) {
        vec2 center = uv - 0.5;
        float vig = smoothstep(0.85, 0.35, length(center));
        color.rgb *= vig;
    }

    if (u_glassGlare) {
        vec2 g = uv - vec2(0.28, 0.22);
        float glare = exp(-dot(g, g) * 9.0) * 0.10;
        color.rgb = clamp(color.rgb + glare, 0.0, 1.0);
    }

    fragColor = color;
}
)";
