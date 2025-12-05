#version 430 core
in vec3 fragColor;
out vec4 color;

uniform vec3 objectColor;
uniform float useVertexColor;  // 0.0 = use objectColor, 1.0 = use vertex color

void main() {
    vec3 finalColor = mix(objectColor, fragColor, useVertexColor);
    color = vec4(finalColor, 1.0);
}
