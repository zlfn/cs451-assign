#version 450 core

struct Complex {
    float re;
    float im;
};

layout(std430, binding = 1) readonly buffer HeightData {
    Complex height[];
};

uniform int uGridSize;
uniform float uHeightScale;

out float vHeight;

void main() {
    int N   = uGridSize;
    int idx = gl_VertexID;   // 0 .. N*N-1

    int x = idx % N;
    int y = idx / N;

    // [-1, 1] 범위로 정규화한 평면 좌표 (XY plane)
    float fx = (float(x) / float(N - 1)) * 2.0 - 1.0;
    float fy = (float(y) / float(N - 1)) * 2.0 - 1.0;

    float h = height[idx].re * uHeightScale;

    // 예전이랑 동일: 평면은 XY, 높이는 Z
    vec3 pos = vec3(fx, fy, h);

    float angle = -1.2; // ~70도
    mat4 rotation = mat4(
        1.0, 0.0,        0.0,        0.0,
        0.0, cos(angle), -sin(angle), 0.0,
        0.0, sin(angle),  cos(angle), 0.0,
        0.0, 0.0,        0.0,        1.0
    );

    gl_Position = rotation * vec4(pos, 1.0);
    gl_PointSize = 3.0;

    vHeight = h;
}
