#version 450 core

struct Complex {
    float re;
    float im;
};

layout(std430, binding = 0) readonly buffer HeightData {
    Complex height[];
};
layout(std430, binding = 1) readonly buffer DXData {
    Complex dxData[];
};
layout(std430, binding = 2) readonly buffer DYData {
    Complex dyData[];
};

uniform int   uIFFTGridSize;     // IFFT resolution (예: 256)
uniform int   uRenderGridSize;   // Render mesh resolution (예: 256)
uniform float uHeightScale;
uniform float uLambda;           // 수평 변위 강도 (choppiness 계수 같은 느낌)
uniform mat4  uView;
uniform mat4  uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out float vHeight;
out vec2 vUV;

// IFFT 격자에서 height 샘플 (wrap 포함)
float sampleHeight(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N; // wrap
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    return height[idx].re;
}

// IFFT 격자에서 DX 샘플 (wrap 포함)
float sampleDX(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N;
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    return dxData[idx].re;
}

// IFFT 격자에서 DY 샘플 (wrap 포함)
// (여기서는 Z방향 수평 변위로 사용)
float sampleDY(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = (gx % N + N) % N;
    gy = (gy % N + N) % N;
    int idx = gy * N + gx;
    return dyData[idx].re;
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

    // ─────────────────────────────
    // 1. IFFT 격자로 매핑 (보간 없음, nearest-like)
    // ─────────────────────────────
    int N = uIFFTGridSize;

    float gx_f = u * float(N);
    float gy_f = v * float(N);
    int   gx   = int(floor(gx_f));
    int   gy   = int(floor(gy_f));

    // height 샘플
    float h = sampleHeight(gx, gy) * uHeightScale;

    // 수평 변위 샘플 (DX, DY) + lambda 적용
    float dispX = sampleDX(gx, gy) * uLambda;
    float dispZ = sampleDY(gx, gy) * uLambda;

    // ─────────────────────────────
    // 2. 노멀 계산 (IFFT 격자 기준 차분)
    //    여기서는 예전처럼 height만으로 계산 (DX/DY는 무시)
    // ─────────────────────────────
    float hL = sampleHeight(gx - 1, gy) * uHeightScale;
    float hR = sampleHeight(gx + 1, gy) * uHeightScale;
    float hD = sampleHeight(gx, gy - 1) * uHeightScale;
    float hU = sampleHeight(gx, gy + 1) * uHeightScale;

    // IFFT 그리드에서 한 칸의 물리적 거리 (월드에서)
    float worldEps = 4.0 / float(N);

    vec3 normal = normalize(vec3(
        hL - hR,
        2.0 * worldEps,
        hD - hU
    ));

    // ─────────────────────────────
    // 3. 최종 위치/출력
    // ─────────────────────────────
    // 평면은 XZ, 높이는 Y
    // base + 수평 변위(DX,DY)
    vec3 pos = vec3(baseX + dispX, h, baseZ + dispZ);

    gl_Position = uProjection * uView * vec4(pos, 1.0);

    vWorldPos = pos;
    vNormal   = normal;
    vHeight   = h;
    vUV       = vec2(u, v);
}
