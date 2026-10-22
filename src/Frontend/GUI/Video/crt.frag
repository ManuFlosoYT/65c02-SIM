#version 130
in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_texture;
uniform sampler2D u_prevFrame;
uniform vec2  u_nativeSize;  // vec2(100.0, 75.0)
uniform vec2  u_outputSize;  // vec2(800.0, 600.0)
uniform float u_time;

// Essentials
uniform bool u_scanlines;
uniform bool u_interlacing;
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
uniform bool u_halation;

// Signal Advanced
uniform bool u_ghosting;
uniform bool u_moire;
uniform float u_gamma;

// 100x75 Specifics
uniform bool u_sharpPixels;
uniform bool u_humBar;
uniform int  u_monitorType;

const float PI = 3.14159265359;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 curveUV(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / vec2(5.5, 4.0);
    uv = uv + uv * offset * offset;
    return uv * 0.5 + 0.5;
}

float roundedCornerDist(vec2 uv, float radius) {
    vec2 c = abs(uv * 2.0 - 1.0) - (1.0 - radius);
    return length(max(c, 0.0)) - radius;
}

// Subtexel pixel sampling for 100x75: maintains the core of the pixel sharp and rounds the transition
vec4 sampleVRAM(vec2 uv) {
    if (!u_sharpPixels) {
        return texture(u_texture, uv);
    }
    vec2 texel = uv * u_nativeSize - 0.5;
    vec2 idx = floor(texel);
    vec2 f = fract(texel);
    // Sharpness curve: 4.0 in X (smooth analog transition horizontal) and 6.0 in Y (sharp between scanlines)
    vec2 sharpF = clamp((f - 0.5) * vec2(4.0, 6.0), -0.5, 0.5) + 0.5;
    sharpF = sharpF * sharpF * (3.0 - 2.0 * sharpF); // Hermite smoothstep
    return texture(u_texture, (idx + sharpF + 0.5) / u_nativeSize);
}

vec3 rgb2yiq(vec3 c) {
    return vec3(
        dot(c, vec3(0.299,  0.587,  0.114)),
        dot(c, vec3(0.596, -0.274, -0.322)),
        dot(c, vec3(0.211, -0.523,  0.312))
    );
}

vec3 yiq2rgb(vec3 c) {
    return vec3(
        dot(c, vec3(1.0,  0.956,  0.621)),
        dot(c, vec3(1.0, -0.272, -0.647)),
        dot(c, vec3(1.0, -1.106,  1.703))
    );
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

void main() {
    vec2 uv = v_uv;
    vec2 nativeTexel = 1.0 / u_nativeSize; // (0.01, 0.01333) para 100x75
    vec2 outputTexel = 1.0 / u_outputSize;
    float bezelMask = 1.0;

    if (u_curvature) {
        uv = curveUV(uv);
        vec2 edge = smoothstep(vec2(0.0), outputTexel * 2.5, uv) *
                    smoothstep(vec2(0.0), outputTexel * 2.5, 1.0 - uv);
        bezelMask *= edge.x * edge.y;
        if (bezelMask <= 0.001) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    if (u_cornerRounding) {
        float dist = roundedCornerDist(uv, 0.07);
        bezelMask *= 1.0 - smoothstep(-0.004, 0.004, dist);
        if (bezelMask <= 0.001) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    vec2 videoUV = uv;

    if (u_vsyncJitter) {
        // Horizontal synchronization with the 75 horizontal scanlines
        float lineIndex = floor(videoUV.y * u_nativeSize.y);
        float band = floor((lineIndex + u_time * 12.0) * 0.25);
        float drift = (hash(vec2(band, floor(u_time * 8.0))) - 0.5) * nativeTexel.x * 0.25;
        videoUV.x += drift;
    }

    // --- 1. Base sampling (calibrated for large 100x75 pixels) ---

    // Radial convergence maximum of ~0.35 100x75 pixels at the corners
    vec2 radialShift = u_chromatic ? (videoUV - 0.5) * nativeTexel * 0.7 : vec2(0.0);
    vec4 color;

    if (u_blur) {
        // 0.35 native texel step for blurring the phosphor without making the text illegible in 100x75
        vec2 blurStep = nativeTexel * 0.35;
        vec3 sum = vec3(0.0);
        float weights[9] = float[](
            0.0625, 0.125, 0.0625,
            0.1250, 0.250, 0.1250,
            0.0625, 0.125, 0.0625
        );
        int idx = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                vec2 off = vec2(float(dx), float(dy)) * blurStep;
                float r = sampleVRAM(videoUV + off + radialShift).r;
                float g = sampleVRAM(videoUV + off).g;
                float b = sampleVRAM(videoUV + off - radialShift).b;
                sum += vec3(r, g, b) * weights[idx++];
            }
        }
        color = vec4(sum, 1.0);
    } else if (u_chromatic) {
        float r = sampleVRAM(videoUV + radialShift).r;
        float g = sampleVRAM(videoUV).g;
        float b = sampleVRAM(videoUV - radialShift).b;
        color = vec4(r, g, b, 1.0);
    } else {
        color = sampleVRAM(videoUV);
    }

    // --- 2. Analog Signal (radii adjusted to the 100x75 scale) ---

    if (u_colorBleeding) {
        // In 100 columns, we sample at 0.45 and 0.90 pixels of horizontal distance
        vec3 yiqCenter = rgb2yiq(color.rgb);
        vec3 yiqL1 = rgb2yiq(sampleVRAM(videoUV - vec2(nativeTexel.x * 0.45, 0.0)).rgb);
        vec3 yiqR1 = rgb2yiq(sampleVRAM(videoUV + vec2(nativeTexel.x * 0.45, 0.0)).rgb);
        vec3 yiqL2 = rgb2yiq(sampleVRAM(videoUV - vec2(nativeTexel.x * 0.90, 0.0)).rgb);
        vec3 yiqR2 = rgb2yiq(sampleVRAM(videoUV + vec2(nativeTexel.x * 0.90, 0.0)).rgb);

        vec2 blurredChroma = yiqCenter.yz * 0.4 + (yiqL1.yz + yiqR1.yz) * 0.2 + (yiqL2.yz + yiqR2.yz) * 0.1;
        color.rgb = clamp(yiq2rgb(vec3(yiqCenter.x, blurredChroma)), 0.0, 1.0);
    }

    if (u_ghosting) {
        // Analog echo at 1.2 and 2.4 pixels to the right within the 100 columns
        vec3 g1 = sampleVRAM(videoUV - vec2(nativeTexel.x * 1.2, 0.0)).rgb * 0.15;
        vec3 g2 = sampleVRAM(videoUV - vec2(nativeTexel.x * 2.4, 0.0)).rgb * 0.07;
        color.rgb = clamp(color.rgb + g1 - g2 * 0.4, 0.0, 1.0);
    }

    // --- 3. Bloom and Halation ---

    if (u_bloom || u_halation) {
        vec3 bloomAcc = vec3(0.0);
        vec3 halationAcc = vec3(0.0);
        float weightSum = 0.0;

        for (int dy = -2; dy <= 2; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                vec2 d = vec2(float(dx), float(dy));
                float w = exp(-dot(d, d) * 0.4);
                weightSum += w;

                if (u_bloom) {
                    // Short Halo (~0.35 texels of 100x75 per step)
                    vec3 sb = texture(u_texture, videoUV + d * nativeTexel * 0.35).rgb;
                    float lumaB = dot(sb, vec3(0.2126, 0.7152, 0.0722));
                    bloomAcc += sb * max(lumaB - 0.5, 0.0) * w;
                }
                if (u_halation) {
                    // Crystal Difusion (~0.85 texels of 100x75 per step)
                    vec3 sh = texture(u_texture, videoUV + d * nativeTexel * 0.85).rgb;
                    float lumaH = dot(sh, vec3(0.2126, 0.7152, 0.0722));
                    halationAcc += sh * pow(lumaH, 3.0) * w;
                }
            }
        }
        if (u_bloom)    color.rgb += (bloomAcc / weightSum) * 0.40;
        if (u_halation) color.rgb += (halationAcc / weightSum) * 0.25;
    }

    // --- 4. Optional Monochrome Phosphor ---

    if (u_monitorType != 0) {
        float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
        if (u_monitorType == 1) {
            color.rgb = vec3(0.12, 1.0, 0.22) * pow(luma, 0.9);
        } else if (u_monitorType == 2) {
            color.rgb = vec3(1.0, 0.68, 0.10) * pow(luma, 0.9);
        } else {
            color.rgb = vec3(0.92, 0.95, 1.0) * luma;
        }
    }

    // --- 5. Shadow Mask (works on the 800x600 subpixels of the FBO) ---

    if (u_shadowMask && u_monitorType == 0) {
        vec2 maskCoord = floor(gl_FragCoord.xy);
        float stagger = mod(floor(maskCoord.y / 2.0), 2.0) * 1.5;
        int px = int(mod(maskCoord.x + stagger, 3.0));

        vec3 mask = vec3(0.74);
        if      (px == 0) mask.r = 1.16;
        else if (px == 1) mask.g = 1.16;
        else              mask.b = 1.16;

        color.rgb *= mask;
    }

    // --- 6. Synchronized Scanlines ---

    // Every integer unit of rowCoord corresponds to 1 of the 75 rows in your VRAM
    float rowCoord = uv.y * u_nativeSize.y;

    if (u_scanlines) {
        float luma = clamp(dot(color.rgb, vec3(0.299, 0.587, 0.114)), 0.0, 1.0);
        // distFromRowCenter = 0.0 on the center of the pixel, 1.0 at the joint between two rows of the 75
        float distFromRowCenter = abs(fract(rowCoord) - 0.5) * 2.0;
        // Beam blooming: the bright pixels widen the vertical beam
        float beamWidth = mix(1.8, 0.75, luma);
        float scanlineProfile = exp(-pow(distFromRowCenter * beamWidth, 2.2));
        // Brightness compensation to avoid darkening the scene
        color.rgb *= mix(scanlineProfile, 1.0, 0.20) * 1.18;
    }

    if (u_interlacing) {
        float field = floor(mod(u_time * 30.0, 2.0));
        float lineParity = mod(floor(rowCoord) + field, 2.0);
        color.rgb *= mix(0.94, 1.06, lineParity);
    }

    if (u_humBar) {
        float humWave = sin((uv.y + u_time * 0.25) * PI * 2.0);
        color.rgb *= 1.0 + smoothstep(0.3, 1.0, humWave) * 0.08;
    }

    if (u_noise) {
        // Noise at subpixel scale but modulated by the 75 lines
        float n = (hash(floor(uv * u_outputSize * 0.5) + vec2(u_time * 7.3, u_time * 13.7)) - 0.5) * 0.06;
        color.rgb = clamp(color.rgb + n, 0.0, 1.0);
    }

    if (u_moire) {
        vec2 moireCoord = uv * u_outputSize * 0.65;
        float m = sin(moireCoord.x) * sin(moireCoord.y);
        color.rgb *= 1.0 + m * 0.04;
    }

    if (u_vignette) {
        vec2 center = uv - 0.5;
        float vig = smoothstep(0.85, 0.35, length(center));
        color.rgb *= vig;
    }

    if (u_gamma != 2.2) {
        color.rgb = pow(max(color.rgb, vec3(0.0)), vec3(u_gamma / 2.2));
    }

    // --- 7. Temporal Persistence of the Phosphor (Ping-Pong FBO) ---

    vec2 gVec = uv - vec2(0.28, 0.22);
    float glare = u_glassGlare ? exp(-dot(gVec, gVec) * 9.0) * 0.10 : 0.0;

    if (u_phosphorDecay) {
        vec3 prev = max(texture(u_prevFrame, v_uv).rgb - vec3(glare), vec3(0.0));
        vec3 decayRate = (u_monitorType == 1 || u_monitorType == 2)
                             ? vec3(0.84, 0.86, 0.80)
                             : vec3(0.72, 0.75, 0.68);
        color.rgb = max(color.rgb, prev * decayRate);
    }

    if (u_glassGlare) {
        color.rgb = clamp(color.rgb + glare, 0.0, 1.0);
    }

    color.rgb *= bezelMask;
    fragColor = vec4(color.rgb, 1.0);
}