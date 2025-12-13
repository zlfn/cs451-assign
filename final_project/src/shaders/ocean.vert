#version 450 core

struct Complex {
    float re;
    float im;
};

layout(std430, binding = 0) readonly buffer SWEHeightData {
    float sweHeight[];
};
layout(std430, binding = 1) readonly buffer TessenHeightData {
    Complex tHeight[];
};
layout(std430, binding = 2) readonly buffer DXData {
    Complex dxData[];
};
layout(std430, binding = 3) readonly buffer DYData {
    Complex dyData[];
};
layout(std430, binding = 4) readonly buffer MaskData {
    float alphaMask[];
};

uniform int   uIFFTGridSize;     // IFFT resolution (예: 128)
uniform int   uRenderGridSize;   // Render mesh resolution (예: 128)
uniform float uHeightScale;
uniform float uLambda;           // 수평 변위 강도 (choppiness 계수 같은 느낌)
uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform int   uCenterTile;

out vec3 vWorldPos;
out vec3 vNormal;
out float vHeight;
out vec2 vUV;
out vec4 vClipSpacePos;
out float vJacobian;

// 격자에서 tHeight 샘플 (wrap 포함)
float sampleHeight(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N; // wrap
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    if (uCenterTile == 1) {
        if (tHeight[idx].re > 0.0) {
            return sweHeight[idx] / uHeightScale;
        } else {
            return sweHeight[idx];
        }
    } else {
        return tHeight[idx].re;
    }
}

// 격자에서 DX 샘플 (wrap 포함)
float sampleDX(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N;
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    return dxData[idx].re;
}

// 격자에서 DY 샘플 (wrap 포함)
// (여기서는 Z방향 수평 변위로 사용)
float sampleDY(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N;
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    return dyData[idx].re;
}

// 격자에서 Mask 샘플 (wrap 포함)
float sampleMask(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N;
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    return alphaMask[idx];
}

void main() {
    int renderN = uRenderGridSize;
    int idx = gl_VertexID;

    int vx = idx % renderN;
    int vy = idx / renderN;

    // 메쉬 상의 [0,1] 정규 좌표
    float u = float(vx) / float(renderN - 1);
    float v = float(vy) / float(renderN - 1);

    // 월드 평면 좌표 (XZ), [-2,2] 범위의 기본 평면
    float baseX = u * 4.0 - 2.0;
    float baseZ = v * 4.0 - 2.0;

    // 1. IFFT 격자로 매핑 (보간 없음, nearest-like)
    int N = uIFFTGridSize;

    float gx_f = u * float(N);
    float gy_f = v * float(N);
    int   gx   = int(floor(gx_f));
    int   gy   = int(floor(gy_f));

    // tHeight 샘플
    float h = sampleHeight(gx, gy) * uHeightScale;

    // 수평 변위 샘플 (DX, DY) + lambda 적용
    float dispX = sampleDX(gx, gy) * uLambda;
    float dispZ = sampleDY(gx, gy) * uLambda;

    // Center Tile인 경우, 섬 근처(mask=0)에서 변위를 제거하여 갭 방지
    if (uCenterTile == 1) {
        float mask = sampleMask(gx, gy);
        dispX *= mask;
        dispZ *= mask;
    }

    // IFFT 그리드에서 한 칸의 물리적 거리 (월드에서)
    float worldEps = 4.0 / float(N);

    // Calculate normal using finite differences of the ACTUAL displaced positions
    // This ensures lighting matches the geometry even with high choppiness (lambda)
    
    // Neighbors' indices
    int gxL = gx - 1; int gxR = gx + 1;
    int gyD = gy - 1; int gyU = gy + 1;

    // Sample neighbor displacements
    // Left
    float hL  = sampleHeight(gxL, gy) * uHeightScale;
    float dxL = sampleDX(gxL, gy) * uLambda;
    float dyL = sampleDY(gxL, gy) * uLambda;
    vec3 posL = vec3(-worldEps + dxL, hL, dyL); // Relative to center base

    // Right
    float hR  = sampleHeight(gxR, gy) * uHeightScale;
    float dxR = sampleDX(gxR, gy) * uLambda;
    float dyR = sampleDY(gxR, gy) * uLambda;
    vec3 posR = vec3(worldEps + dxR, hR, dyR);

    // Down (assuming v goes up?) 
    // In OpenGL texture coords, usually V=0 is bottom, V=1 is top.
    // Here v = vy / (renderN-1). vy increases -> v increases -> baseZ increases.
    // So gy-1 is "down" in indices, "lower" in Z (if we map v to Z).
    // Let's check baseZ = v * 4.0 - 2.0. So higher vy = higher Z.
    // So gyD (gy-1) is -Z direction relative to gy.
    float hD  = sampleHeight(gx, gyD) * uHeightScale;
    float dxD = sampleDX(gx, gyD) * uLambda;
    float dyD = sampleDY(gx, gyD) * uLambda;
    vec3 posD = vec3(dxD, hD, -worldEps + dyD);

    // Up
    float hU  = sampleHeight(gx, gyU) * uHeightScale;
    float dxU = sampleDX(gx, gyU) * uLambda;
    float dyU = sampleDY(gx, gyU) * uLambda;
    vec3 posU = vec3(dxU, hU, worldEps + dyU);

    // Tangent (X direction)
    vec3 tangent = posR - posL;
    // Bitangent (Z direction)
    vec3 bitangent = posU - posD;

    // Normal
    vec3 normal = normalize(cross(bitangent, tangent)); // Z cross X = Y (Up)

    // Jacobian calculation for foam
    float dxDX = sampleDX(gxR, gy) - sampleDX(gxL, gy);
    float dxDZ = sampleDX(gx, gyU) - sampleDX(gx, gyD);
    float dzDX = sampleDY(gxR, gy) - sampleDY(gxL, gy);
    float dzDZ = sampleDY(gx, gyU) - sampleDY(gx, gyD);

    float dxDX_norm = dxDX * uLambda / (2.0 * worldEps);
    float dxDZ_norm = dxDZ * uLambda / (2.0 * worldEps);
    float dzDX_norm = dzDX * uLambda / (2.0 * worldEps);
    float dzDZ_norm = dzDZ * uLambda / (2.0 * worldEps);

    float jacobian = (1.0 + dxDX_norm) * (1.0 + dzDZ_norm) - dxDZ_norm * dzDX_norm;

    // 최종 위치. 평면은 XZ, 높이는 Y
    vec3 pos = vec3(baseX + dispX, h, baseZ + dispZ);

    vWorldPos = (uModel * vec4(pos, 1.0)).xyz;
    vec4 clipPos = uProjection * uView * vec4(vWorldPos, 1.0);
    gl_Position = clipPos;

    vClipSpacePos = clipPos;
    vNormal   = normal;
    vHeight   = h;
    vUV       = vec2(u, v);
    vJacobian = jacobian;
}
