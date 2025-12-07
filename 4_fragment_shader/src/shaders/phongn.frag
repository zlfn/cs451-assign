#version 430 core

#define MAX_LIGHTS 4

struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float intensity;
    vec3 attenuation;
    int enabled;
};

uniform Light lights[MAX_LIGHTS];
uniform int numLights;

uniform vec3 objectColor;
uniform float useTexture;
uniform sampler2D colorSampler;
uniform sampler2D normalSampler;

in vec3 vertPosition_VS;
in vec3 vertNormal_VS;
in vec2 vertTexCoord;

out vec4 finalColor;

// Calculate TBN matrix using derivatives (no pre-computed tangents needed)
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv) {
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    // solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame
    float maxTB = max(dot(T,T), dot(B,B));

    // If UV derivatives are too small, return identity-like TBN (just use original normal)
    if (maxTB < 1e-6) {
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, N));
        vec3 bitangent = cross(N, tangent);
        return mat3(tangent, bitangent, N);
    }

    float invmax = inversesqrt(maxTB);
    return mat3(T * invmax, B * invmax, N);
}

void main() {
    vec3 N_VS = normalize(vertNormal_VS);
    vec3 V_VS = normalize(-vertPosition_VS);

    // Perturb normal
    // 1. Get normal from map [0,1] -> [-1,1]
    vec3 mapNormal = texture(normalSampler, vertTexCoord).rgb;
    mapNormal = mapNormal * 2.0 - 1.0;
    
    // 2. Calculate TBN
    mat3 TBN = cotangent_frame(N_VS, vertPosition_VS, vertTexCoord);
    
    // 3. Transform map normal to View Space
    vec3 normal_VS = normalize(TBN * mapNormal);

    vec4 texColor = vec4(1.0);
    if (useTexture > 0.5) {
        texColor = texture(colorSampler, vertTexCoord);
    } else {
        texColor = vec4(objectColor, 1.0);
    }

    vec3 totalAmbient = vec3(0.0);
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);
    float shininess = 32.0;

    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i].enabled == 0) continue;

        vec3 lightAmbient = lights[i].ambient;
        vec3 lightDiffuse = lights[i].diffuse;
        vec3 lightSpecular = lights[i].specular;
        float intensity = lights[i].intensity;
        
        vec3 lightDir_VS;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // Directional
             lightDir_VS = normalize(-lights[i].direction);
        } else { // Point
             lightDir_VS = normalize(lights[i].position - vertPosition_VS);
             float distance = length(lights[i].position - vertPosition_VS);
             attenuation = 1.0 / (lights[i].attenuation.x + lights[i].attenuation.y * distance + lights[i].attenuation.z * distance * distance);
        }

        // Ambient
        totalAmbient += lightAmbient * intensity * attenuation;

        // Diffuse
        float diff = max(dot(normal_VS, lightDir_VS), 0.0);
        totalDiffuse += lightDiffuse * diff * intensity * attenuation;

        // Specular
        vec3 reflectDir = reflect(-lightDir_VS, normal_VS);
        float spec = pow(max(dot(V_VS, reflectDir), 0.0), shininess);
        totalSpecular += lightSpecular * spec * intensity * attenuation;
    }

    vec3 result = (totalAmbient + totalDiffuse) * texColor.rgb + totalSpecular;
    finalColor = vec4(result, texColor.a);
}
