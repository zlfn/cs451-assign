#version 430 core

in vec2 fragTexCoord;
out vec4 color;

uniform sampler2D currentFrame;
uniform sampler2D previousFrame;
uniform float blendFactor; // 0.0 ~ 1.0, higher = more blur

void main() {
    vec4 current = texture(currentFrame, fragTexCoord);
    vec4 previous = texture(previousFrame, fragTexCoord);

    // Blend current frame with previous frame for motion blur effect
    color = mix(current, previous, blendFactor);
}
