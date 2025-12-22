#version 450 core

in vec3 vWorldPos;
in vec3 vNormal;
in float vHeight;
in vec2 vUV;
in vec4 vClipSpacePos;
in float vJacobian;

out vec4 FragColor;

uniform float uTime;

// PBR uniforms
uniform vec3 uCameraPos;
uniform vec3 uLightDir; // 빛 방향
uniform vec3 uLightColor;
uniform float uRoughness;

// refraction & environment
uniform sampler2D uRefractionTexture;
uniform float uRefractionStrength;
uniform samplerCube uEnvironmentMap;
uniform sampler2D uBubbleTexture;
uniform sampler2D uNormalMap;

const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz NDF
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0001);
}

// Smith Schlick-GGX geometry function
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Schlick Fresnel approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Fresnel with roughness for environment reflection
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Hash function for noise
vec3 hash3(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

// 3D Simplex-like noise
float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(mix(dot(hash3(i + vec3(0.0, 0.0, 0.0)), f - vec3(0.0, 0.0, 0.0)),
                       dot(hash3(i + vec3(1.0, 0.0, 0.0)), f - vec3(1.0, 0.0, 0.0)), u.x),
                   mix(dot(hash3(i + vec3(0.0, 1.0, 0.0)), f - vec3(0.0, 1.0, 0.0)),
                       dot(hash3(i + vec3(1.0, 1.0, 0.0)), f - vec3(1.0, 1.0, 0.0)), u.x), u.y),
               mix(mix(dot(hash3(i + vec3(0.0, 0.0, 1.0)), f - vec3(0.0, 0.0, 1.0)),
                       dot(hash3(i + vec3(1.0, 0.0, 1.0)), f - vec3(1.0, 0.0, 1.0)), u.x),
                   mix(dot(hash3(i + vec3(0.0, 1.0, 1.0)), f - vec3(0.0, 1.0, 1.0)),
                       dot(hash3(i + vec3(1.0, 1.0, 1.0)), f - vec3(1.0, 1.0, 1.0)), u.x), u.y), u.z);
}

// FBM for turbulence
float fbm(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

void main() {
    // Normal Perturbation with Normal Map
    vec2 normalUV1 = vWorldPos.xz * 3.5 + vec2(uTime * 0.02, uTime * 0.015);
    vec2 normalUV2 = vWorldPos.xz * 6.0 - vec2(uTime * 0.025, uTime * 0.02);
    vec2 normalUV3 = vWorldPos.xz * 2.0 + vec2(uTime * 0.01, -uTime * 0.012);

    vec3 normalTex1 = texture(uNormalMap, normalUV1).rgb * 2.0 - 1.0;
    vec3 normalTex2 = texture(uNormalMap, normalUV2).rgb * 2.0 - 1.0;
    vec3 normalTex3 = texture(uNormalMap, normalUV3).rgb * 2.0 - 1.0;

    vec3 detailNormal = normalize(normalTex1 * 0.5 + normalTex2 * 0.3 + normalTex3 * 0.2);

    vec3 N = normalize(vNormal + vec3(detailNormal.x, 0.0, detailNormal.y) * 0.55);

    // 2. View & Light Vectors
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(uLightDir);
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    // Water F0 (refractive index 1.33 -> F0 = ((1.33-1)/(1.33+1))^2 = 0.02)
    vec3 F0 = vec3(0.02);

    // Refraction with Chromatic Aberration & Depth Simulation
    vec2 distortion = N.xz * uRefractionStrength * 3.0;
    vec2 floorUV = (vWorldPos.xz * 0.1) + distortion;

    // Chromatic aberration
    float aberration = 0.003;
    float r = texture(uRefractionTexture, floorUV - aberration).r;
    float g = texture(uRefractionTexture, floorUV).g;
    float b = texture(uRefractionTexture, floorUV + aberration).b;
    vec3 floorColor = vec3(r, g, b);

    // Water color blending with depth simulation
    vec3 deepWaterColor = vec3(0.0, 0.01, 0.04); // Very dark navy
    vec3 shallowWaterColor = vec3(0.02, 0.15, 0.25); // Dark teal

    // Depth factor: looking down = transparent, looking horizon = opaque
    float depthFactor = 1.0 - NdotV;
    float opacity = mix(0.3, 0.95, depthFactor);

    vec3 refractionColor = mix(floorColor * shallowWaterColor * 2.5, deepWaterColor, opacity);

    // Cook-Torrance Specular BRDF
    float NDF = DistributionGGX(N, H, uRoughness);
    float G = GeometrySmith(N, V, L, uRoughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // Oren-Nayar Diffuse BRDF
    float sigma2 = uRoughness * uRoughness;
    float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.33);
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    float thetaI = acos(NdotL);
    float thetaR = acos(NdotV);
    float alpha = max(thetaI, thetaR);
    float beta = min(thetaI, thetaR);

    // Azimuth difference approximation
    vec3 lightProj = normalize(L - N * NdotL);
    vec3 viewProj = normalize(V - N * NdotV);
    float cosPhi = max(dot(lightProj, viewProj), 0.0);

    // Water base color (very dark blue for physical accuracy)
    vec3 waterColor = vec3(0.01, 0.03, 0.08);

    float orenNayar = A + B * cosPhi * sin(alpha) * tan(beta);
    vec3 diffuse = (1.0 - F) * waterColor / PI * orenNayar;

    // Environment Reflection (Cubemap with Fresnel)
    vec3 R = reflect(-V, N);
    vec3 envColor = textureLod(uEnvironmentMap, R, uRoughness * 5.0).rgb;

    // Fresnel for environment reflection
    vec3 F_env = fresnelSchlickRoughness(NdotV, F0, uRoughness);
    vec3 envReflection = F_env * envColor * 1.2; // Boosted for realism

    // Subsurface Scattering (Physical)
    vec3 sssColor = vec3(0.08, 0.35, 0.55);  // Realistic oceanic SSS
    float waveThickness = clamp(1.0 - abs(vHeight) * 2.0, 0.0, 1.0);

    // Light extinction in water
    vec3 extinction = vec3(0.35, 0.18, 0.12);  // RGB absorption coefficients
    vec3 transmittance = exp(-extinction * (1.0 - waveThickness) * 2.5);

    // Forward & back scattering
    float LdotV = dot(L, -V);
    float forwardScatter = pow(max(LdotV * 0.5 + 0.5, 0.0), 4.0);
    float backScatter = pow(max(dot(N, L) * 0.5 + 0.5, 0.0), 2.0);

    // Height-based SSS (more at crests)
    float heightSSS = smoothstep(-0.1, 0.4, vHeight);

    float sssFactor = (forwardScatter * 0.5 + backScatter * 0.4) * waveThickness;
    sssFactor *= heightSSS * 0.7;
    vec3 sss = sssColor * transmittance * sssFactor * uLightColor * NdotL;

    // Translucency (Wave Crest Glow)
    float crestHeight = smoothstep(0.0, 0.35, vHeight);
    float backLight = max(dot(-N, L), 0.0);
    vec3 translucencyColor = vec3(0.12, 0.38, 0.52);
    vec3 translucency = translucencyColor * crestHeight * backLight * uLightColor * 2.2;

    // Foam from Jacobian
    float jacobianNoise = fbm(vWorldPos * 12.0 + vec3(uTime * 0.15), 3) * 0.3;
    float adjustedJacobian = vJacobian + jacobianNoise;

    float foamBase = 1.0 - smoothstep(0.2, 0.8, adjustedJacobian);

    float detailNoise1 = fbm(vWorldPos * 8.0 + vec3(uTime * 0.2), 2);
    float detailNoise2 = fbm(vWorldPos * 20.0 - vec3(uTime * 0.3), 2);
    float combinedNoise = detailNoise1 * 0.6 + detailNoise2 * 0.4;
    combinedNoise = combinedNoise * 0.5 + 0.5;

    float foamAmount = foamBase * combinedNoise;
    foamAmount = pow(foamAmount, 1.5);

    vec2 flowDir1 = vec2(uTime * 0.03, uTime * 0.02);
    vec2 flowDir2 = vec2(-uTime * 0.025, uTime * 0.035);
    vec2 flowDir3 = vec2(uTime * 0.015, -uTime * 0.028);

    vec2 uvDistort = vec2(
        fbm(vWorldPos * 5.0 + vec3(uTime * 0.1), 2),
        fbm(vWorldPos * 5.0 + vec3(uTime * 0.1, 100.0, 0.0), 2)
    ) * 0.05;

    vec2 bubbleUV1 = vWorldPos.xz * 0.8 + flowDir1 + uvDistort;
    vec2 bubbleUV2 = vWorldPos.xz * 1.2 + flowDir2 - uvDistort * 0.5;
    vec2 bubbleUV3 = vWorldPos.xz * 0.6 + flowDir3 + uvDistort * 0.7;

    float bubble1 = texture(uBubbleTexture, bubbleUV1).r;
    float bubble2 = texture(uBubbleTexture, bubbleUV2).r;
    float bubble3 = texture(uBubbleTexture, bubbleUV3).r;

    float timeVar = sin(uTime * 0.5) * 0.5 + 0.5;
    float bubbleIntensity = mix(
        bubble1 * 0.5 + bubble2 * 0.3 + bubble3 * 0.2,
        bubble2 * 0.5 + bubble3 * 0.3 + bubble1 * 0.2,
        timeVar
    );

    vec3 foamColor = vec3(1.0);
    float foamAlpha = bubbleIntensity * foamAmount;

    // Final Composition
    vec3 color = vec3(0.0);

    // Mix refraction and environment reflection based on Fresnel
    vec3 baseColor = mix(refractionColor, envReflection, F_env);

    color += baseColor;                           // Base (refraction + env reflection)
    color += diffuse * uLightColor * NdotL;       // Oren-Nayar diffuse
    color += specular * uLightColor * NdotL;      // Cook-Torrance specular
    color += sss;                                 // Subsurface scattering
    color += translucency;                        // Wave crest translucency

    // Ambient occlusion from wave troughs
    float ao = clamp(vHeight + 0.5, 0.2, 1.0);
    color *= ao;

    // Add foam where waves fold
    color = mix(color, foamColor, foamAlpha * 0.85);

    // Tone Mapping & Gamma Correction. ACES filmic tone mapping
    color = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
