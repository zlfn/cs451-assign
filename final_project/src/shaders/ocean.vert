#version 450 core

struct Complex {
    float re;
    float im;
};

layout(std430, binding = 0) readonly buffer SWEHeightData {
    float sweHeight[];          // SWE height(eta or displacement)
};
layout(std430, binding = 1) readonly buffer TessenHeightData {
    Complex tHeight[];          // Tessendorf IFFT height (spatial domain) in .re
};
layout(std430, binding = 2) readonly buffer DXData {
    Complex dxData[];           // Tessendorf displacement X (spatial) in .re
};
layout(std430, binding = 3) readonly buffer DYData {
    Complex dyData[];           // Tessendorf displacement Z (spatial) in .re
};
layout(std430, binding = 4) readonly buffer MaskData {
    float alphaMask[];          // 0 near island, 1 far ocean (assumed)
};

uniform int   uIFFTGridSize;     // IFFT resolution (e.g., 128)
uniform int   uRenderGridSize;   // Render mesh resolution (e.g., 128)
uniform float uHeightScale;      // final vertical scale
uniform float uLambda;           // horizontal displacement strength
uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform int   uCenterTile;       // 1: center tile uses SWE near island, blend outward

out vec3  vWorldPos;
out vec3  vNormal;
out float vHeight;
out vec2  vUV;
out vec4  vClipSpacePos;
out float vJacobian;

// ---------- Integer sampling (wrap) ----------
int wrapIndex(int a, int N) { return (a % N + N) % N; }

int idxFromGrid(int gx, int gy) {
    int N = uIFFTGridSize;
    gx = wrapIndex(gx, N);
    gy = wrapIndex(gy, N);
    return gy * N + gx;
}

float sampleSWEHeightInt(int gx, int gy) {
    int idx = idxFromGrid(gx, gy);
    return sweHeight[idx];
}

float sampleTessHeightInt(int gx, int gy) {
    int idx = idxFromGrid(gx, gy);
    return tHeight[idx].re;
}

float sampleDXInt(int gx, int gy) {
    int idx = idxFromGrid(gx, gy);
    return dxData[idx].re;
}

float sampleDYInt(int gx, int gy) {
    int idx = idxFromGrid(gx, gy);
    return dyData[idx].re;
}

float sampleMaskInt(int gx, int gy) {
    int idx = idxFromGrid(gx, gy);
    return alphaMask[idx];
}

// ---------- Bilinear sampling on [0,1] using N-1 mapping ----------
void gridCoordsNMinus1(float u, float v, out float gx_f, out float gy_f) {
    int N = uIFFTGridSize;
    float Nm1 = float(max(N - 1, 1));
    gx_f = u * Nm1;
    gy_f = v * Nm1;
}

float bilinearGeneric(float u, float v,
                      float s00, float s10, float s01, float s11,
                      float fx, float fy)
{
    float sx0 = mix(s00, s10, fx);
    float sx1 = mix(s01, s11, fx);
    return mix(sx0, sx1, fy);
}

float bilinearSWEHeight(float u, float v) {
    float gx_f, gy_f;
    gridCoordsNMinus1(u, v, gx_f, gy_f);

    int x0 = int(floor(gx_f));
    int y0 = int(floor(gy_f));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = gx_f - float(x0);
    float fy = gy_f - float(y0);

    float h00 = sampleSWEHeightInt(x0, y0);
    float h10 = sampleSWEHeightInt(x1, y0);
    float h01 = sampleSWEHeightInt(x0, y1);
    float h11 = sampleSWEHeightInt(x1, y1);

    return bilinearGeneric(u, v, h00, h10, h01, h11, fx, fy);
}

float bilinearTessHeight(float u, float v) {
    float gx_f, gy_f;
    gridCoordsNMinus1(u, v, gx_f, gy_f);

    int x0 = int(floor(gx_f));
    int y0 = int(floor(gy_f));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = gx_f - float(x0);
    float fy = gy_f - float(y0);

    float h00 = sampleTessHeightInt(x0, y0);
    float h10 = sampleTessHeightInt(x1, y0);
    float h01 = sampleTessHeightInt(x0, y1);
    float h11 = sampleTessHeightInt(x1, y1);

    return bilinearGeneric(u, v, h00, h10, h01, h11, fx, fy);
}

float bilinearDX(float u, float v) {
    float gx_f, gy_f;
    gridCoordsNMinus1(u, v, gx_f, gy_f);

    int x0 = int(floor(gx_f));
    int y0 = int(floor(gy_f));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = gx_f - float(x0);
    float fy = gy_f - float(y0);

    float d00 = sampleDXInt(x0, y0);
    float d10 = sampleDXInt(x1, y0);
    float d01 = sampleDXInt(x0, y1);
    float d11 = sampleDXInt(x1, y1);

    return bilinearGeneric(u, v, d00, d10, d01, d11, fx, fy);
}

float bilinearDY(float u, float v) {
    float gx_f, gy_f;
    gridCoordsNMinus1(u, v, gx_f, gy_f);

    int x0 = int(floor(gx_f));
    int y0 = int(floor(gy_f));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = gx_f - float(x0);
    float fy = gy_f - float(y0);

    float d00 = sampleDYInt(x0, y0);
    float d10 = sampleDYInt(x1, y0);
    float d01 = sampleDYInt(x0, y1);
    float d11 = sampleDYInt(x1, y1);

    return bilinearGeneric(u, v, d00, d10, d01, d11, fx, fy);
}

float bilinearMask(float u, float v) {
    float gx_f, gy_f;
    gridCoordsNMinus1(u, v, gx_f, gy_f);

    int x0 = int(floor(gx_f));
    int y0 = int(floor(gy_f));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = gx_f - float(x0);
    float fy = gy_f - float(y0);

    float m00 = sampleMaskInt(x0, y0);
    float m10 = sampleMaskInt(x1, y0);
    float m01 = sampleMaskInt(x0, y1);
    float m11 = sampleMaskInt(x1, y1);

    return bilinearGeneric(u, v, m00, m10, m01, m11, fx, fy);
}

void main() {
    int renderN = uRenderGridSize;
    int idx = gl_VertexID;

    int vx = idx % renderN;
    int vy = idx / renderN;

    // [0,1] including boundaries
    float u = float(vx) / float(max(renderN - 1, 1));
    float v = float(vy) / float(max(renderN - 1, 1));

    // base plane in local space: [-2,2]
    float baseX = u * 4.0 - 2.0;
    float baseZ = v * 4.0 - 2.0;

    // Mask (0 near island, 1 far ocean) for center tile blending
    float a = 1.0;
    if (uCenterTile == 1) {
        a = clamp(bilinearMask(u, v), 0.0, 1.0);
    }

    // --- Height: continuous blend SWE <-> Tessendorf (NO sign-based branching) ---
    float h_swe  = bilinearSWEHeight(u, v);
    float h_tess = bilinearTessHeight(u, v);

    // Convention:
    // a=0 (island vicinity) -> use SWE
    // a=1 (far ocean)      -> use Tessendorf
    float h_raw = mix(h_swe, h_tess, a);
    float h     = h_raw * uHeightScale;

    // --- Displacement (from Tessendorf) ---
    float dispX = bilinearDX(u, v) * uLambda;
    float dispZ = bilinearDY(u, v) * uLambda; // DY used as Z displacement (as before)

    // Prevent "almost fixed to 0" near island:
    // keep some displacement even when mask is near 0 (tunable)
    if (uCenterTile == 1) {
        const float minKeep = 0.25;          // 0.0 -> original (fully killed), 0.2~0.4 recommended
        float m = mix(minKeep, 1.0, a);
        dispX *= m;
        dispZ *= m;
    }

    // --- Sampling step in world units ---
    int N = uIFFTGridSize;
    float worldEps = 4.0 / float(max(N - 1, 1));  // because base plane spans 4 units

    // --- Normal via central differences (consistent positions in SAME coordinate frame) ---
    float du = 1.0 / float(max(N - 1, 1));
    float dv = 1.0 / float(max(N - 1, 1));

    float uL = fract(u - du + 1.0);
    float uR = fract(u + du);
    float vD = fract(v - dv + 1.0);
    float vU = fract(v + dv);

    // Heights blended consistently at neighbors
    float aL = (uCenterTile == 1) ? clamp(bilinearMask(uL, v ), 0.0, 1.0) : 1.0;
    float aR = (uCenterTile == 1) ? clamp(bilinearMask(uR, v ), 0.0, 1.0) : 1.0;
    float aD = (uCenterTile == 1) ? clamp(bilinearMask(u , vD), 0.0, 1.0) : 1.0;
    float aU = (uCenterTile == 1) ? clamp(bilinearMask(u , vU), 0.0, 1.0) : 1.0;

    float hL_raw = mix(bilinearSWEHeight(uL, v ), bilinearTessHeight(uL, v ), aL);
    float hR_raw = mix(bilinearSWEHeight(uR, v ), bilinearTessHeight(uR, v ), aR);
    float hD_raw = mix(bilinearSWEHeight(u , vD), bilinearTessHeight(u , vD), aD);
    float hU_raw = mix(bilinearSWEHeight(u , vU), bilinearTessHeight(u , vU), aU);

    float hL = hL_raw * uHeightScale;
    float hR = hR_raw * uHeightScale;
    float hD = hD_raw * uHeightScale;
    float hU = hU_raw * uHeightScale;

    // Displacement neighbors (Tess only, but mask-attenuated consistently)
    float dxL = bilinearDX(uL, v ) * uLambda;
    float dzL = bilinearDY(uL, v ) * uLambda;
    float dxR = bilinearDX(uR, v ) * uLambda;
    float dzR = bilinearDY(uR, v ) * uLambda;
    float dxD = bilinearDX(u , vD) * uLambda;
    float dzD = bilinearDY(u , vD) * uLambda;
    float dxU = bilinearDX(u , vU) * uLambda;
    float dzU = bilinearDY(u , vU) * uLambda;

    if (uCenterTile == 1) {
        const float minKeep = 0.25;
        float mL = mix(minKeep, 1.0, aL);
        float mR = mix(minKeep, 1.0, aR);
        float mD = mix(minKeep, 1.0, aD);
        float mU = mix(minKeep, 1.0, aU);

        dxL *= mL; dzL *= mL;
        dxR *= mR; dzR *= mR;
        dxD *= mD; dzD *= mD;
        dxU *= mU; dzU *= mU;
    }

    // Build neighbor positions in the SAME local plane frame (baseX/baseZ included)
    vec3 posL = vec3((baseX - worldEps) + dxL, hL, baseZ + dzL);
    vec3 posR = vec3((baseX + worldEps) + dxR, hR, baseZ + dzR);
    vec3 posD = vec3(baseX + dxD, hD, (baseZ - worldEps) + dzD);
    vec3 posU = vec3(baseX + dxU, hU, (baseZ + worldEps) + dzU);

    vec3 tangent   = posR - posL;
    vec3 bitangent = posU - posD;
    vec3 nLocal    = normalize(cross(bitangent, tangent));

    // Transform normal properly (handles non-uniform scale in uModel)
    mat3 Nmat = transpose(inverse(mat3(uModel)));
    vec3 nWorld = normalize(Nmat * nLocal);

    // --- Jacobian for foam (central difference, world-normalized) ---
    // Use raw dx/dy fields (before lambda) and normalize by worldEps
    float DxL0 = bilinearDX(uL, v );
    float DxR0 = bilinearDX(uR, v );
    float DxD0 = bilinearDX(u , vD);
    float DxU0 = bilinearDX(u , vU);

    float DzL0 = bilinearDY(uL, v );
    float DzR0 = bilinearDY(uR, v );
    float DzD0 = bilinearDY(u , vD);
    float DzU0 = bilinearDY(u , vU);

    float dxDX_norm = (DxR0 - DxL0) * uLambda / (2.0 * worldEps);
    float dxDZ_norm = (DxU0 - DxD0) * uLambda / (2.0 * worldEps);
    float dzDX_norm = (DzR0 - DzL0) * uLambda / (2.0 * worldEps);
    float dzDZ_norm = (DzU0 - DzD0) * uLambda / (2.0 * worldEps);

    float jacobian = (1.0 + dxDX_norm) * (1.0 + dzDZ_norm) - dxDZ_norm * dzDX_norm;

    // --- Final vertex position --
    vec3 posLocal = vec3(baseX + dispX, h, baseZ + dispZ);
    float shore = (uCenterTile==1) ? (1.0 - a) : 0.0;
    posLocal.y -= 0.002 * shore;

    vec4 worldPos4 = uModel * vec4(posLocal, 1.0);
    vWorldPos = worldPos4.xyz;

    vec4 clipPos = uProjection * uView * worldPos4;
    gl_Position = clipPos;

    vClipSpacePos = clipPos;
    vNormal   = nWorld;
    vHeight   = h;
    vUV       = vec2(u, v);
    vJacobian = jacobian;
}
