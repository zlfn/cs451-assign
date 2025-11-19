#version 450 core

struct Complex {
    float re;
    float im;
};

layout(std430, binding = 1) readonly buffer HeightData {
    Complex height[];
};

uniform int uIFFTGridSize;     // IFFT resolution (e.g., 32)
uniform int uRenderGridSize;   // Render mesh resolution (e.g., 256)
uniform float uHeightScale;
uniform mat4 uView;
uniform mat4 uProjection;

out float vHeight;

// Cubic interpolation weight function (Catmull-Rom)
float cubicWeight(float x) {
    float ax = abs(x);
    if (ax < 1.0) {
        return 1.0 - 2.5*ax*ax + 1.5*ax*ax*ax;
    } else if (ax < 2.0) {
        return 2.0 - 4.0*ax + 2.5*ax*ax - 0.5*ax*ax*ax;
    }
    return 0.0;
}

// Sample height from IFFT grid with wrapping
float sampleHeight(int gx, int gy) {
    int N = uIFFTGridSize;
    // Wrap coordinates for seamless tiling
    gx = (gx + N) % N;
    gy = (gy + N) % N;
    int idx = gy * N + gx;
    return height[idx].re;
}

// Bicubic interpolation of height field
float bicubicInterpolate(float u, float v) {
    int N = uIFFTGridSize;

    // Convert normalized coords to grid space
    float gx = u * float(N);
    float gy = v * float(N);

    // Integer grid cell
    int gxi = int(floor(gx));
    int gyi = int(floor(gy));

    // Fractional part within cell
    float fx = gx - float(gxi);
    float fy = gy - float(gyi);

    // Sample 4x4 neighborhood and apply bicubic weights
    float h = 0.0;
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -1; dx <= 2; dx++) {
            float wx = cubicWeight(float(dx) - fx);
            float wy = cubicWeight(float(dy) - fy);
            h += sampleHeight(gxi + dx, gyi + dy) * wx * wy;
        }
    }

    return h;
}

void main() {
    int renderN = uRenderGridSize;
    int idx = gl_VertexID;

    int x = idx % renderN;
    int y = idx / renderN;

    // Normalized coordinates [0, 1] in the mesh
    float u = float(x) / float(renderN - 1);
    float v = float(y) / float(renderN - 1);

    // World space position before height displacement
    float fx = u * 4.0 - 2.0;  // [-2, 2]
    float fz = v * 4.0 - 2.0;  // [-2, 2]

    // Sample height using bicubic interpolation
    float h = bicubicInterpolate(u, v) * uHeightScale;

    // 평면은 XZ (horizontal), 높이는 Y (up)
    vec3 pos = vec3(fx, h, fz);

    // Apply view and projection matrices
    gl_Position = uProjection * uView * vec4(pos, 1.0);

    vHeight = h;
}
