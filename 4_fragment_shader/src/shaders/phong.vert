#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 inputColor;

uniform mat4 projection;
uniform mat4 modelView;
uniform mat4 normalMatrix;

out vec3 vertPosition_VS;
out vec3 vertNormal_VS;
out vec3 vertColor;

void main() {
    vec4 pos_VS = modelView * vec4(position, 1.0);
    vertPosition_VS = pos_VS.xyz;
    vertNormal_VS = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
    vertColor = inputColor;
    gl_Position = projection * pos_VS;
}