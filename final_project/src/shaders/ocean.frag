#version 450 core

in float vHeight;
out vec4 FragColor;

// 높이 값을 컬러로 매핑
vec3 heightToColor(float t) {
    float value = (t * 0.5) + 0.5; // -1~1 -> 0~1 (대충)

    vec3 low  = vec3(0.0, 0.0, 1.0);  // 파랑
    vec3 mid  = vec3(0.0, 1.0, 0.0);  // 초록
    vec3 high = vec3(1.0, 0.0, 0.0);  // 빨강

    vec3 color = mix(low, mid, value * 2.0);
    if (value > 0.5) {
        color = mix(mid, high, (value - 0.5) * 2.0);
    }
    return color;
}

void main() {
    FragColor = vec4(heightToColor(vHeight * 100.0), 1.0);
}
