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

in vec3 vertPosition_VS;
in vec3 vertNormal_VS;
in vec2 vertTexCoord;

out vec4 finalColor;

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
        float spec = pow(max(dot(viewVec_VS, reflectDir), 0.0), shininess);
        totalSpecular += lightSpecular * spec * intensity * attenuation;
    }

    // Environment reflection
    vec3 reflectDir_VS = reflect(-viewVec_VS, normal_VS);
    vec3 reflectDir_WS = mat3(inverseViewMatrix) * reflectDir_VS;
    vec3 envColor = texture(environmentMap, reflectDir_WS).rgb;

    // Combine with environment reflection
    vec3 baseColor = (totalAmbient + totalDiffuse) * texColor.rgb + totalSpecular;
    vec3 result = mix(baseColor, envColor, reflectivity);
    finalColor = vec4(result, texColor.a);
}
