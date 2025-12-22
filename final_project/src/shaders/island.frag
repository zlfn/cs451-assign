#version 450 core

in float vHeight;
in vec3 vNormal;
in vec2 vUV;
in vec3 vWorldPos;
in vec3 vTangent;
in vec3 vBitangent;

out vec4 FragColor;

uniform float uHeightScale;
uniform vec3 uCameraPos;
uniform sampler2D uNormalMap;

const float PI = 3.14159265359;

// PBR parameters for stone
const vec3 stoneBaseColor = vec3(0.45, 0.45, 0.47);  // Gray stone color
const float stoneRoughness = 0.85;  // Rough stone surface
const float stoneMetallic = 0.05;   // Non-metallic

// Light direction (from above)
const vec3 lightDir = normalize(vec3(0.3, 1.0, 0.2));
const vec3 lightColor = vec3(1.0, 0.98, 0.95);  // Warm sunlight

// Fresnel approximation (Schlick)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX Normal Distribution Function
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Geometry function (Smith's Schlick-GGX)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

void main()
{
    // Sample normal map and convert from [0,1] to [-1,1]
    vec3 normalMap = texture(uNormalMap, vUV * 4.0).rgb * 2.0 - 1.0;

    // Build TBN matrix to transform normal from tangent space to world space
    vec3 T = normalize(vTangent);
    vec3 B = normalize(vBitangent);
    vec3 N_base = normalize(vNormal);
    mat3 TBN = mat3(T, B, N_base);

    // Transform normal map to world space
    vec3 N = normalize(TBN * normalMap);

    // View direction
    vec3 V = normalize(uCameraPos - vWorldPos);

    // PBR Lighting calculation
    vec3 L = lightDir;
    vec3 H = normalize(V + L);

    // Base reflectivity (F0) - for non-metals use 0.04
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, stoneBaseColor, stoneMetallic);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, stoneRoughness);
    float G = geometrySmith(N, V, L, stoneRoughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // kS is equal to Fresnel
    vec3 kS = F;
    // For energy conservation, diffuse and specular light can't be above 1.0
    vec3 kD = vec3(1.0) - kS;
    // Multiply kD by the inverse metalness such that only non-metals have diffuse lighting
    kD *= 1.0 - stoneMetallic;

    // Scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

    // Outgoing radiance Lo
    vec3 Lo = (kD * stoneBaseColor / PI + specular) * lightColor * NdotL;

    // Ambient lighting (simple IBL approximation)
    vec3 ambient = vec3(0.12) * stoneBaseColor;

    vec3 color = ambient + Lo;

    // HDR tonemapping (simple Reinhard)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
