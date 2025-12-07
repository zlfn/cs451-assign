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
uniform int numLights; // Not strictly used if checking enabled

uniform vec3 objectColor;
uniform float useTexture;
uniform sampler2D colorSampler;
uniform float materialShininess; // e.g. 32.0

// Environment reflection
uniform samplerCube environmentMap;
uniform float reflectivity; // 0.0 ~ 1.0
uniform mat4 inverseViewMatrix;

// Shadow mapping
uniform sampler2DShadow shadowMap;
uniform int shadowEnabled;

in vec3 vertPosition_VS;
in vec3 vertNormal_VS;
in vec2 vertTexCoord;
in vec4 vertPosition_LS;

out vec4 finalColor;

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if outside shadow map - be more lenient
    if (projCoords.z > 1.0) {
        return 0.0; // Beyond far plane, no shadow
    }
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0; // Outside shadow map bounds
    }

    // Calculate bias based on surface angle to light (larger bias for point lights)
    float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.0005);
    float currentDepth = projCoords.z - bias;

    // PCF (Percentage Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, currentDepth);
            shadow += texture(shadowMap, sampleCoord);
        }
    }
    shadow /= 25.0; // 5x5 PCF kernel

    return 1.0 - shadow; // Return shadow factor (1 = fully shadowed, 0 = lit)
}

void main() {
    vec3 normal_VS = normalize(vertNormal_VS);
    vec3 viewVec_VS = normalize(-vertPosition_VS);

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

    // Calculate shadow for point light (second light, index 1)
    float shadow = 0.0;
    if (shadowEnabled == 1 && lights[1].enabled == 1 && lights[1].type == 1) {
        vec3 lightDir_VS = normalize(lights[1].position - vertPosition_VS);
        shadow = calculateShadow(vertPosition_LS, normal_VS, lightDir_VS);
    }

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

        // Ambient (not affected by shadow)
        totalAmbient += lightAmbient * intensity * attenuation;

        // Apply shadow only to point light (index 1)
        float shadowFactor = (i == 1) ? (1.0 - shadow) : 1.0;

        // Diffuse
        float diff = max(dot(normal_VS, lightDir_VS), 0.0);
        totalDiffuse += lightDiffuse * diff * intensity * attenuation * shadowFactor;

        // Specular
        vec3 reflectDir = reflect(-lightDir_VS, normal_VS);
        float spec = pow(max(dot(viewVec_VS, reflectDir), 0.0), shininess);
        totalSpecular += lightSpecular * spec * intensity * attenuation * shadowFactor;
    }

    // Environment reflection
    vec3 reflectDir_VS = reflect(-viewVec_VS, normal_VS);
    vec3 reflectDir_WS = mat3(inverseViewMatrix) * reflectDir_VS;
    vec3 envColor = texture(environmentMap, reflectDir_WS).rgb;

    // Combine with environment reflection
    vec3 baseColor = (totalAmbient + totalDiffuse) * texColor.rgb + totalSpecular;
    vec3 result = mix(baseColor, envColor, reflectivity);

    // Debug: visualize shadow (uncomment to debug)
    // Uncomment the line below to see shadow map coverage
    // result = vec3(1.0 - shadow);

    // Visualize light space position (for debugging)
    // vec3 projCoords = vertPosition_LS.xyz / vertPosition_LS.w;
    // projCoords = projCoords * 0.5 + 0.5;
    // result = vec3(projCoords.z); // Depth visualization

    finalColor = vec4(result, texColor.a);
}
