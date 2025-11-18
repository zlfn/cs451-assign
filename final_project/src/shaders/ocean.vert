#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aHeight;
out float vHeight;

void main() {
    // Scale height to make waves visible
    vec3 pos = vec3(aPos.xy, aHeight * 5.0f);

    // Apply a static rotation around the X-axis for a better viewing angle
    float angle = -1.2f; // ~70 degrees
    mat4 rotation = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, cos(angle), -sin(angle), 0.0,
        0.0, sin(angle), cos(angle), 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    gl_Position = rotation * vec4(pos, 1.0);
    
    vHeight = aHeight;
}
