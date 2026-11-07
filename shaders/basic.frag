#version 330 core

#define MAX_LIGHTS 12
#define MAX_POLY_POINTS 240

in vec2 vUV;
in vec3 vWorldPos;
in vec3 vNormal;

out vec4 FragColor;

uniform sampler2D uTex;
uniform vec3 uFogColor;
uniform vec3 uCameraPos;
uniform float uFogStart;
uniform float uFogEnd;
uniform int uLightCount;
uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];
uniform float uLightRadius[MAX_LIGHTS];
uniform float uLightIntens[MAX_LIGHTS];
uniform int uLightPolyN[MAX_LIGHTS];
uniform float uLightPoly[MAX_LIGHTS * MAX_POLY_POINTS];
uniform vec3 uAmbient;
uniform int uLightingQuality;
uniform float uMaterialLight;
uniform float uEmissive;
uniform float uTime;
uniform int uFlashlightOn;
uniform vec3 uFlashlightDir;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

void main() {
    vec4 base = texture(uTex, vUV);
    vec3 N = normalize(vNormal);
    vec3 color;

    if (uEmissive > 0.5) {
        // Лампы остаются светлыми, но не пересвечивают потолок.
        color = base.rgb * vec3(1.05, 1.03, 0.96);
    } else {
        // Убираем чрезмерный жёлтый сдвиг исходных текстур.
        base.rgb *= vec3(0.99, 0.97, 0.91);
        vec3 lighting = uAmbient;
        float bounce = max(0.0, N.y) * 0.045;
        lighting += vec3(bounce) * vec3(0.85, 0.82, 0.72);

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            if (i >= uLightCount) break;

            vec2 rel = vWorldPos.xz - uLightPos[i].xz;
            float dist = length(rel);
            float maxRadius = uLightRadius[i] * 1.2;
            if (dist > maxRadius) continue;

            float shadow = 1.0;
            int polyN = (uLightingQuality == 0) ? 0 : uLightPolyN[i];
            if (polyN > 0) {
                float ang = atan(rel.y, rel.x);
                if (ang < 0.0) ang += 6.2831853;
                float idx = ang / 6.2831853 * float(polyN);
                int i1 = int(floor(idx)) % polyN;
                int i2 = (i1 + 1) % polyN;
                float frac = idx - floor(idx);
                float d1 = uLightPoly[i * MAX_POLY_POINTS + i1];
                float d2 = uLightPoly[i * MAX_POLY_POINTS + i2];
                float boundary = mix(d1, d2, frac);
                float edge = 3.0;
                shadow = clamp((boundary - dist) / edge, 0.0, 1.0);
                shadow = mix(0.4, 1.0, shadow);
            }

            vec3 L = normalize(uLightPos[i] - vWorldPos);
            float NdotL = max(dot(N, L), 0.0);
            float t = 1.0 - (dist / maxRadius);
            float atten = t * t;
            vec3 lampColor = uLightColor[i] * vec3(0.96, 0.98, 0.88);

            // Снижаем интенсивность, сохраняя заметные световые пятна.
            float currentIntensity = uLightIntens[i] * 0.52;
            float timeNoise = hash(floor(uTime * 2.5) + float(i) * 17.0);
            if (timeNoise > 0.95) {
                currentIntensity *= 0.3;
            } else if (timeNoise > 0.85) {
                currentIntensity *= 0.7;
            } else {
                currentIntensity *= (0.97 + 0.03 * sin(uTime * 5.0));
            }

            lighting += lampColor * currentIntensity * NdotL * atten * shadow;
        }

        if (uFlashlightOn == 1) {
            vec3 cameraToFrag = normalize(vWorldPos - uCameraPos);
            vec3 flashlightDir = normalize(uFlashlightDir);
            float spotEffect = dot(cameraToFrag, flashlightDir);
            // Узкий, мягкий конус вместо почти равномерного освещения комнаты.
            float spotIntensity = smoothstep(0.86, 0.97, spotEffect);

            if (spotIntensity > 0.0) {
                float distToCam = length(uCameraPos - vWorldPos);
                float attenuation = 1.0 / (1.0 + 0.06 * distToCam + 0.018 * distToCam * distToCam);
                float rangeFade = 1.0 - smoothstep(13.0, 19.0, distToCam);
                vec3 flashlightColor = vec3(1.0, 0.97, 0.90);
                vec3 L = normalize(uCameraPos - vWorldPos);
                float NdotL = max(dot(N, L), 0.0);
                float lightAmount = NdotL * 0.88 + 0.08;
                lighting += flashlightColor * spotIntensity * attenuation * rangeFade * lightAmount * 1.25;
            }
        }

        color = base.rgb * lighting * uMaterialLight;
    }

    float distCam = length(vWorldPos - uCameraPos);
    float fogFactor = clamp((distCam - uFogStart) / max(uFogEnd - uFogStart, 0.001), 0.0, 1.0);
    fogFactor = pow(fogFactor, 1.2);
    color = mix(color, uFogColor, fogFactor);
    FragColor = vec4(color, 1.0);
}