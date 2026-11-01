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

    // Лёгкое дрожание изображения по горизонтали
    float jitter = (rand(vec2(uTime, uv.y * 5.0)) - 0.5) * 0.0035;
    uv.x += jitter;

    // Редкая tracking-полоса
    float band = fract(uTime * 0.3 + uv.y * 2.0);
    float bandMask = smoothstep(0.985, 1.0, band);
    uv.x += bandMask * (rand(vec2(uTime, 1.0)) - 0.5) * 0.03;

    // Хроматическая аберрация
    float aberr = 0.003 + bandMask * 0.01;
    vec2 rUV = uv + vec2( aberr, 0.0);
    vec2 gUV = uv;
    vec2 bUV = uv + vec2(-aberr, 0.0);

    float r = texture(uScene, rUV).r;
    float g = texture(uScene, gUV).g;
    float b = texture(uScene, bUV).b;
    vec3 color = vec3(r, g, b);

    // Color bleed вправо
    vec3 bleed = texture(uScene, uv + vec2(0.002, 0.0)).rgb;
    color = mix(color, bleed, 0.15);

    // Scanlines
    float scanline = sin(uv.y * uResolutionY * 1.6) * 0.5 + 0.5;
    scanline = mix(1.0, 0.88 + 0.12 * scanline, 0.35);
    color *= scanline;

    // Шум / зерно
    float noise = rand(uv * vec2(uResolutionX, uResolutionY) * 0.5 + uTime);
    color += (noise - 0.5) * 0.08;

    // --- НОВАЯ ЦВЕТОКОРРЕКЦИЯ (ГРЯЗНЫЙ LEVEL 0) ---

    // 1. Делаем картинку менее насыщенной (убираем яркий "мультяшный" желтый)
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(color, vec3(gray), 0.3);

    // 2. Накладываем болезненный оливково-зеленый оттенок (как на старых пленках)
    vec3 tintColor = vec3(0.85, 0.9, 0.5);
    color = mix(color, color * tintColor, 0.5);

    // 3. Чуть повышаем контраст, чтобы вытянуть темные провалы
    color = pow(color, vec3(1.1));

    // Виньетка
    vec2 vUV2 = vUV * 2.0 - 1.0;
    float vignette = 1.0 - dot(vUV2 * 0.85, vUV2 * 0.85);
    vignette = clamp(vignette, 0.0, 1.0);
    color *= mix(0.55, 1.0, vignette);

    FragColor = vec4(color, 1.0);
}