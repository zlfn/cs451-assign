#version 450 core

in vec3 vWorldPos;
in vec3 vNormal;
in float vHeight;
in vec2 vUV;

out vec4 FragColor;

uniform float uTime;

// PBR uniforms
uniform vec3 uCameraPos;
uniform vec3 uLightDir;      // Direction TO light (normalized)
uniform vec3 uLightColor;
uniform float uRoughness;

// IBL
uniform samplerCube uEnvironmentMap;

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

// Smith's Schlick-GGX geometry function
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

// FBM (Fractal Brownian Motion) for turbulence
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
    // Turbulence: perturb normal with high-frequency noise
    vec3 noisePos = vWorldPos * 40.0 + vec3(uTime * 0.3);
    float turbulenceX = fbm(noisePos, 3) * 0.5;
    float turbulenceZ = fbm(noisePos + vec3(100.0), 3) * 0.5;
    vec3 N = normalize(vNormal + 0.5 * vec3(turbulenceX, 0.0, turbulenceZ));

    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(uLightDir);
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    // Water F0 (Fresnel reflectance) is known as 0.02-0.04
    // Water's Refractive index (n) of visible light is 1.33
    // F0 = ((n-1)/(n+1))^2
    vec3 F0 = vec3(0.02);

    /// Specular (Cook-Torrance BRDF) //////////////////////////////////////////
    float NDF = DistributionGGX(N, H, uRoughness);
    float G = GeometrySmith(N, V, L, uRoughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    /// Oren-Nayar Diffuse BRDF ////////////////////////////////////////////////
    float sigma2 = uRoughness * uRoughness;
    float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.33);
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    float thetaI = acos(NdotL);
    float thetaR = acos(NdotV);
    float alpha = max(thetaI, thetaR);
    float beta = min(thetaI, thetaR);

    // Approximate azimuth difference
    vec3 lightProj = normalize(L - N * NdotL);
    vec3 viewProj = normalize(V - N * NdotV);
    float cosPhi = max(dot(lightProj, viewProj), 0.0);

    // Water's real color is very dark blue
    vec3 waterColor = vec3(0.02, 0.05, 0.10);

    float orenNayar = A + B * cosPhi * sin(alpha) * tan(beta);
    vec3 diffuse = (1 - F) * waterColor / PI * orenNayar;

    /// Environemnt Reflection (IBL) ///////////////////////////////////////////

    // Fresnel for environment reflection
    vec3 F_env = fresnelSchlickRoughness(NdotV, F0, uRoughness);

    // Reflect view vector for environment lookup
    vec3 R = reflect(-V, N);

    // Sample environment map with reflection vector
    // Apply roughness-based mip level for approximate pre-filtered reflection
    float maxMipLevel = 5.0;  // Adjust based on cubemap mip levels
    float mipLevel = uRoughness * maxMipLevel;
    vec3 envColor = textureLod(uEnvironmentMap, R, mipLevel).rgb;

    vec3 envReflection = F_env * envColor;

    /// Subsurface scattering (SSS) ////////////////////////////////////////////
    vec3 sssColor = vec3(0.1, 0.4, 0.6);  // Realistic blue SSS color
    float waveThickness = clamp(1.0 - abs(vHeight) * 2.0, 0.0, 1.0);

    // Transmittance based on wave thickness
    vec3 extinction = vec3(0.3, 0.15, 0.1);  // More absorption
    vec3 transmittance = exp(-extinction * (1.0 - waveThickness) * 2.5);

    // Forward scatter term (light coming through from behind)
    float LdotV = dot(L, -V);
    float forwardScatter = pow(max(LdotV * 0.5 + 0.5, 0.0), 4.0);

    // Back scatter term (light bouncing inside water)
    float backScatter = pow(max(dot(N, L) * 0.5 + 0.5, 0.0), 2.0);

    // Height-based SSS intensity (more SSS at wave crests)
    float heightSSS = smoothstep(-0.1, 0.4, vHeight);

    // Combine SSS components (reduced intensity)
    float sssFactor = (forwardScatter * 0.5 + backScatter * 0.4) * waveThickness;
    sssFactor *= heightSSS * 0.6;
    vec3 sss = sssColor * transmittance * sssFactor * uLightColor * NdotL;

    /// Translucency effect for wave crests ////////////////////////////////////
    float crestHeight = smoothstep(0.0, 0.3, vHeight);
    float backLight = max(dot(-N, L), 0.0);
    vec3 translucencyColor = vec3(0.15, 0.35, 0.5);
    vec3 translucency = translucencyColor * crestHeight * backLight * uLightColor * 2.0;

    // Combine all lighting
    vec3 color = vec3(0.0);
    color += diffuse * uLightColor * NdotL;   // Diffuse
    color += specular * uLightColor;          // Specular
    color += envReflection;                   // Environment reflection
    color += sss;                             // Subsurface scattering
    color += translucency;                    // Wave crest translucency

    // Ambient occlusion approximation from wave troughs
    float ao = clamp(vHeight + 0.5, 0.15, 1.0);
    color *= ao;

    // HDR tonemapping (ACES-like)
    color = color * (2.51 * color + 0.03) / (color * (2.43 * color + 0.59) + 0.14);

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
