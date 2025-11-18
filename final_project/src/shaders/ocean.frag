#version 430 core

in float vHeight; // 정점 셰이더에서 전달받은 높이
out vec4 fragcolor;

// 높이 값(t)을 색상(red-green-blue)으로 매핑하는 함수
vec3 heightToColor(float t) {
    // 높이 값을 -1 ~ 1 범위에서 0 ~ 1 범위로 정규화 (범위는 데이터에 맞게 조절)
    float value = (t * 0.5) + 0.5;

    // 간단한 jet 컬러맵 (blue -> green -> red)
    vec3 low = vec3(0.0, 0.0, 1.0);  // blue (낮음)
    vec3 mid = vec3(0.0, 1.0, 0.0);  // green (중간)
    vec3 high = vec3(1.0, 0.0, 0.0); // red (높음)
    
    vec3 color = mix(low, mid, value * 2.0);
    if (value > 0.5) {
        color = mix(mid, high, (value - 0.5) * 2.0);
    }
    return color;
}

void main() {
    // 높이 값에 따라 색상 결정
    fragcolor = vec4(heightToColor(vHeight*100), 1.0);
}
