#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color; // Background passes color here

uniform mat4 projection;
uniform mat4 modelView;

out vec3 fragColor;
out vec2 fragTexCoord; // Dummy

void main() {
    gl_Position = projection * modelView * vec4(position, 1.0);
    fragColor = color;
    fragTexCoord = vec2(0.0);
}
