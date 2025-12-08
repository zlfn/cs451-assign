#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 modelView;
uniform mat4 normalMatrix;
uniform mat4 lightSpaceViewMatrix; // lightSpace * inverseView - transforms from view space to light clip space

out vec3 vertPosition_VS;
out vec3 vertNormal_VS;
out vec2 vertTexCoord;
out vec4 vertPosition_LS; // Light space position for shadow mapping

void main() {
    vec4 pos_VS = modelView * vec4(position, 1.0);
    vertPosition_VS = pos_VS.xyz;
    vertNormal_VS = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
    vertTexCoord = texCoord;

    // Calculate position in light space for shadow mapping
    // lightSpaceViewMatrix already combines lightSpaceMatrix * inverseViewMatrix
    vertPosition_LS = lightSpaceViewMatrix * pos_VS;

    gl_Position = projection * pos_VS;
}
