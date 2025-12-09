#version 450 core

in vec3 vWorldPos;
in vec3 vNormal;
in float vHeight;
in vec2 vUV;
in vec4 vClipSpacePos;

out vec4 FragColor;

uniform float uTime;

// PBR uniforms
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uRoughness;
uniform sampler2D uRefractionTexture;
uniform float uRefractionStrength;    // 굴절 강도

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
    // 1. [순서 변경] 노이즈 및 Normal 계산을 가장 먼저 수행
    // 그래야 굴절과 반사 모두 자글자글한 디테일이 적용됨
    vec3 noisePos = vWorldPos * 40.0 + vec3(uTime * 0.3);
    float turbulenceX = fbm(noisePos, 3) * 0.5; 
    float turbulenceZ = fbm(noisePos + vec3(100.0), 3) * 0.5;
    
    // vNormal 대신 섭동된 N을 구함
    vec3 N = normalize(vNormal + 0.5 * vec3(turbulenceX, 0.0, turbulenceZ));

    // [Moved Up] 3. 조명 벡터 준비 (Needed for Depth Simulation in Step 2)
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(uLightDir);
    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    // 2. 굴절 (Refraction)
    // [개선] vNormal 대신 위에서 구한 N을 사용하여 디테일 추가
    vec2 distortion = N.xz * uRefractionStrength * 3.0; // Reduced from 5.0 for more realistic refraction scale
    
    // Distortion should not depend on height clamping.
    // Using world position for floor UVs
    vec2 floorUV = (vWorldPos.xz * 0.1) + distortion; // 0.1 is tiling factor

    // 색 수차 (Chromatic Aberration) - approximated by offset
    float aberration = 0.003;
    float r = texture(uRefractionTexture, floorUV - aberration).r;
    float g = texture(uRefractionTexture, floorUV).g;
    float b = texture(uRefractionTexture, floorUV + aberration).b;
    vec3 floorColor = vec3(r, g, b);

    // [Water Color Physics]
    // Darker Deep Ocean Look
    vec3 deepWaterColor = vec3(0.0, 0.005, 0.03); // Very Dark Navy (almost black)
    vec3 shallowWaterColor = vec3(0.02, 0.2, 0.3); // Darker Teal

    // [Depth Simulation]
    // Dynamic Opacity based on View Angle (Fresnel-like volume effect)
    // 0.0 (Looking Down) -> "Thin" water -> More Transparent
    // 1.0 (Looking Horizon) -> "Thick" water -> More Opaque
    // mix(Transparency, Opacity, ViewAngleFactor)
    float depthFactor = 1.0 - NdotV; // 0 looking down, 1 at horizon
    float opacity = mix(0.4, 0.95, depthFactor); 

    // Mix the floor texture with the water color. 
    vec3 refractionColor = mix(floorColor * shallowWaterColor * 2.0, deepWaterColor, opacity);

    // 4. Specular (태양광)
    vec3 F0 = vec3(0.02); 
    float NDF = DistributionGGX(N, H, uRoughness);
    float G = GeometrySmith(N, V, L, uRoughness);
    
    // Specular lobe Fresnel (for direct lighting term)
    vec3 F_sun = fresnelSchlick(max(dot(H, V), 0.0), F0); 

    vec3 numerator = NDF * G * F_sun;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // 5. Environment Reflection (하늘)
    vec3 R = reflect(-V, N);
    vec3 envColor = textureLod(uEnvironmentMap, R, uRoughness * 5.0).rgb;
    // Tone down sky reflection slightly to let water color show through
    envColor *= 0.8; 

    // Fresnel for Mix (Reflection vs Refraction)
    // This depends on N and V (viewing angle)
    vec3 F_view = fresnelSchlick(NdotV, F0);

    // 6. SSS (Translucency)
    float crestFactor = smoothstep(0.0, 0.5, vHeight);
    float viewScatter = pow(max(dot(V, -L), 0.0), 4.0);
    float lightScatter = max(dot(-N, L), 0.0) * 0.5 + 0.5;
    
    // Optimized SSS for realistic wave glowing
    vec3 sssColor = vec3(0.1, 0.5, 0.6); // Brighter teal
    vec3 sss = sssColor * (viewScatter + lightScatter) * crestFactor * uLightColor * 1.5;

    // 7. [중요] 최종 합성
    // Mix refraction and reflection based on View Fresnel
    vec3 color = mix(refractionColor, envColor, F_view);
    
    // Add Sun Specular (Boosted for sparkle)
    color += specular * uLightColor * 2.0; 
    
    // Add SSS
    color += sss; 

    // HDR Tone mapping & Gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}
