#version 430 core

#define LIGHT_POS_VS     vec3(0.0, 5.0, 0.0)
#define AMBIENT_COLOR    vec3(0.1, 0.1, 0.1)
#define LIGHT_COLOR      vec3(0.5, 0.5, 0.5)
#define SHININESS        8.0

struct Light {
    vec3 pos;
    vec3 color;
    float intensity;
};

uniform int pointLightCount;
uniform Light pointLights[10];
uniform Light directionLight;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 inputColor;

uniform mat4 projection;
uniform mat4 modelView;
uniform mat4 normalMatrix;

out vec3 fragColor;

void main() {
    vec4 pos_VS = modelView * vec4(position, 1.0);
    vec3 vertexPos_VS = pos_VS.xyz;
    vec3 normal_VS = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
    vec3 viewVec_VS = normalize(-vertexPos_VS);

    vec3 lightVec_VS = normalize(LIGHT_POS_VS - vertexPos_VS); // point light

    float diff = max(dot(normal_VS, lightVec_VS), 0.0); // diffuse
    vec3 diffuse = LIGHT_COLOR * diff * inputColor;
    vec3 reflectVec_VS = reflect(-lightVec_VS, normal_VS); // specular
    float spec = pow(max(dot(viewVec_VS, reflectVec_VS), 0.0), SHININESS);
    vec3 specular = LIGHT_COLOR * spec;

    for (int i = 0; i < pointLightCount; i++) {
        float diff = max(dot(normal_VS, lightVec_VS), 0.0); // diffuse
        diffuse += LIGHT_COLOR * diff * inputColor;

        vec3 reflectVec_VS = reflect(-lightVec_VS, normal_VS); // specular
        float spec = pow(max(dot(viewVec_VS, reflectVec_VS), 0.0), SHININESS);
        specular += LIGHT_COLOR * spec;
    }

    vec3 ambient = AMBIENT_COLOR * inputColor;
    fragColor = ambient + diffuse + specular;
    gl_Position = projection * pos_VS;
}