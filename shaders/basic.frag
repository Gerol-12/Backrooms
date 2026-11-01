#version 330 core

#define MAX_LIGHTS 12
#define MAX_POLY_POINTS 240

in vec2 vUV;
in vec3 vWorldPos;
in vec3 vNormal;

out vec4 FragColor;

uniform sampler2D uTex;

uniform vec3  uFogColor;
uniform vec3  uCameraPos;
uniform float uFogStart;
uniform float uFogEnd;

uniform int   uLightCount;
uniform vec3  uLightPos   [MAX_LIGHTS];
uniform vec3  uLightColor [MAX_LIGHTS];
uniform float uLightRadius[MAX_LIGHTS];
uniform float uLightIntens[MAX_LIGHTS];

uniform int   uLightPolyN   [MAX_LIGHTS];
uniform float uLightPoly    [MAX_LIGHTS * MAX_POLY_POINTS];

uniform vec3  uAmbient;
uniform float uEmissive;
uniform float uTime;

uniform int   uFlashlightOn;
uniform vec3  uFlashlightDir;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

void main() {
    vec4 base = texture(uTex, vUV);
    vec3 N = normalize(vNormal);
    vec3 color;

    if (uEmissive > 0.5) {
        color = base.rgb * vec3(1.3, 1.2, 0.9);
    } else {
        base.rgb *= vec3(1.05, 0.98, 0.75);
        vec3 lighting = uAmbient;
        float bounce = max(0.0, N.y) * 0.15;
        lighting += vec3(bounce) * vec3(0.8, 0.7, 0.5);

        // --- СВЕТ ОТ ЛАМП ---
        for (int i = 0; i < MAX_LIGHTS; ++i) {
            if (i >= uLightCount) break;

            vec2 rel  = vWorldPos.xz - uLightPos[i].xz;
            float dist = length(rel);

            float maxRadius = uLightRadius[i] * 1.2;
            if (dist > maxRadius) continue;

            float shadow = 1.0;
            int polyN = uLightPolyN[i];
            if (polyN > 0) {
                float ang = atan(rel.y, rel.x);
                if (ang < 0.0) ang += 6.2831853;
                float idx  = ang / 6.2831853 * float(polyN);
                int   i1   = int(floor(idx)) % polyN;
                int   i2   = (i1 + 1) % polyN;
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

            vec3 lampColor = uLightColor[i] * vec3(0.9, 1.0, 0.6);

            float currentIntensity = uLightIntens[i] * 1.5;
            float timeNoise = hash(floor(uTime * 2.5) + float(i) * 17.0);
            if (timeNoise > 0.95) {
                currentIntensity *= 0.3;
            } else if (timeNoise > 0.85) {
                currentIntensity *= 0.7;
            } else {
                currentIntensity *= (0.95 + 0.05 * sin(uTime * 5.0));
            }

            lighting += lampColor * currentIntensity * NdotL * atten * shadow;
        }

        // --- СВЕТ ОТ ФОНАРИКА ---
        if (uFlashlightOn == 1) {
            vec3 cameraToFrag = normalize(vWorldPos - uCameraPos);
            vec3 flashlightDir = normalize(uFlashlightDir);

            float spotEffect = dot(cameraToFrag, flashlightDir);

            // Широкий конус (задевает стены по бокам)
            float outerCutoff = 0.2;
            float innerCutoff = 0.85;
            float spotIntensity = smoothstep(outerCutoff, innerCutoff, spotEffect);

            if (spotIntensity > 0.0) {
                float distToCam = length(uCameraPos - vWorldPos);
                float attenuation = 1.0 / (1.0 + 0.03 * distToCam + 0.005 * distToCam * distToCam);

                vec3 flashlightColor = vec3(1.0, 0.95, 0.8);
                vec3 L = normalize(uCameraPos - vWorldPos);
                float NdotL = max(dot(N, L), 0.0);

                float lightAmount = (NdotL * 0.7 + 0.3);

                lighting += flashlightColor * spotIntensity * attenuation * lightAmount * 2.0;
            }
        }

        color = base.rgb * lighting;
    }

    float distCam = length(vWorldPos - uCameraPos);
    float fogFactor = clamp((distCam - uFogStart) / max(uFogEnd - uFogStart, 0.001), 0.0, 1.0);
    fogFactor = pow(fogFactor, 1.2);
    color = mix(color, uFogColor, fogFactor);

    FragColor = vec4(color, 1.0);
}