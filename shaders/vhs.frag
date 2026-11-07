#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform float uTime;
uniform float uResolutionX;
uniform float uResolutionY;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 uv = vUV;
    float jitter = (rand(vec2(uTime, uv.y * 5.0)) - 0.5) * 0.0035;
    uv.x += jitter;

    float band = fract(uTime * 0.3 + uv.y * 2.0);
    float bandMask = smoothstep(0.985, 1.0, band);
    uv.x += bandMask * (rand(vec2(uTime, 1.0)) - 0.5) * 0.03;

    float aberr = 0.002 + bandMask * 0.006;
    float r = texture(uScene, uv + vec2(aberr, 0.0)).r;
    float g = texture(uScene, uv).g;
    float b = texture(uScene, uv + vec2(-aberr, 0.0)).b;
    vec3 color = vec3(r, g, b);

    vec3 bleed = texture(uScene, uv + vec2(0.002, 0.0)).rgb;
    color = mix(color, bleed, 0.10);

    float scanline = sin(uv.y * uResolutionY * 1.6) * 0.5 + 0.5;
    scanline = mix(1.0, 0.90 + 0.10 * scanline, 0.30);
    color *= scanline;

    float noise = rand(uv * vec2(uResolutionX, uResolutionY) * 0.5 + uTime);
    color += (noise - 0.5) * 0.045;

    // Более нейтральная VHS-палитра: меньше кислотно-жёлтого и зелёного.
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(color, vec3(gray), 0.38);
    vec3 tintColor = vec3(0.93, 0.94, 0.84);
    color = mix(color, color * tintColor, 0.30);
    color = pow(max(color, vec3(0.0)), vec3(1.08));

    vec2 vUV2 = vUV * 2.0 - 1.0;
    float vignette = clamp(1.0 - dot(vUV2 * 0.85, vUV2 * 0.85), 0.0, 1.0);
    color *= mix(0.58, 1.0, vignette);

    FragColor = vec4(color, 1.0);
}