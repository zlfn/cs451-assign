#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 projection;  // Projection matrix
uniform mat4 modelView;   // Model-View matrix

out vec3 fragColor;

void main() {
    gl_Position = projection * modelView * vec4(position, 1.0);
    fragColor = color;
}
