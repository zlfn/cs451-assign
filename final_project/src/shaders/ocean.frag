#version 450 core

in float vHeight;
out vec4 FragColor;

// 높이 값을 컬러로 매핑
vec3 heightToColor(float h) {
    // Normalize height to 0~1 range based on expected wave amplitude
    // Adjust the divisor based on actual wave height range
    float value = clamp((h / 2.0) + 0.5, 0.0, 1.0);

    vec3 low  = vec3(0.0, 0.2, 0.5);  // 어두운 파랑 (낮음)
    vec3 mid  = vec3(0.0, 0.6, 0.8);  // 밝은 파랑 (중간)
    vec3 high = vec3(0.8, 0.95, 1.0); // 거품 흰색 (높음)

    vec3 color;
    if (value < 0.5) {
        color = mix(low, mid, value * 2.0);
    } else {
        color = mix(mid, high, (value - 0.5) * 1.0);
    }
    return color;
}

void main() {
    FragColor = vec4(heightToColor(vHeight * 100.0), 1.0);
}
