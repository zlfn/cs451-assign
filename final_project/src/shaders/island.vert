#version 450 core

uniform int uGridSize;
uniform int uRenderGridSize;
uniform float uHeightScale;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out float vHeight;
out vec2 vUV;
out vec3 vTangent;
out vec3 vBitangent;

layout(std430, binding = 0) readonly buffer HeightData {
    float height[];
};

// 격자에서 height 샘플
float sampleHeight(int gx, int gy) {
    gx = (gx % uGridSize + uGridSize) % uGridSize; 
    gy = (gy % uGridSize + uGridSize) % uGridSize;
    int idx = gy * uGridSize + gx;
    return height[idx];
}

void main() {
    int idx = gl_VertexID;

    // 2D 격자 좌표 계산
    int vx = idx % uRenderGridSize;
    int vy = idx / uRenderGridSize;

    // 단위 격자 좌표
    float u = float(vx) / float(uRenderGridSize - 1);
    float v = float(vy) / float(uRenderGridSize - 1);
    float gx_f = u * float(uGridSize); 
    float gy_f = v * float(uGridSize);
    int gx = int(floor(gx_f));
    int gy = int(floor(gy_f));

    // normal 벡터 계산 (격자 기준 중앙 차분)
    float hL = sampleHeight(gx - 1, gy) * uHeightScale;
    float hR = sampleHeight(gx + 1, gy) * uHeightScale;
    float hD = sampleHeight(gx, gy - 1) * uHeightScale;
    float hU = sampleHeight(gx, gy + 1) * uHeightScale;
    float worldEps = 4.0 / float(uGridSize); 
    float dH_dx = (hR - hL) / (2.0 * worldEps); // R - L
    float dH_dz = (hU - hD) / (2.0 * worldEps); // U - D
    vec3 normal = normalize(vec3(-dH_dx, 1.0, -dH_dz));

    // tangent space basis vectors for normal mapping
    vec3 tangent = normalize(vec3(1.0, dH_dx, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dH_dz, 1.0));

    // 최종 위치 출력
    float baseX = u * 4.0 - 2.0;
    float h = sampleHeight(gx, gy) * uHeightScale;
    float baseZ = v * 4.0 - 2.0;
    vec3 pos = vec3(baseX, h, baseZ);
    vWorldPos = pos;
    gl_Position = uProjection * uView * vec4(vWorldPos, 1.0);

    vNormal = normal;
    vTangent = tangent;
    vBitangent = bitangent;
    vHeight = h;
    vUV = vec2(u, v);
}