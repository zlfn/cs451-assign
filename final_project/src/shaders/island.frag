#version 450 core

in float vHeight; 
in vec3 vNormal; // 필요시 조명 계산에 사용
in vec2 vUV; // 필요시 텍스처링에 사용

out vec4 FragColor;

uniform float uHeightScale;

// 높이 임계값 정의
const float waterLevel = 0.1;
const float grassLevel = 0.3;
const float rockLevel  = 0.6;
const float snowLevel  = 0.9;

void main()
{
    // 높이 정규화
    float normalizedHeight = vHeight / uHeightScale;
    vec3 color;

    // 색 지정
    if (normalizedHeight < waterLevel) { // 물 (가장 낮은 지대)
        color = vec3(0.1, 0.3, 0.8); // 파란색
    } else if (normalizedHeight < grassLevel) { // 해변 또는 낮은 풀
        color = vec3(0.8, 0.7, 0.4); // 모래색
    } else if (normalizedHeight < rockLevel) { // 풀 지대
        color = vec3(0.2, 0.6, 0.1); // 녹색
    } else if (normalizedHeight < snowLevel) { // 바위 지대
        color = vec3(0.5, 0.5, 0.5); // 회색
    } else { // 눈 지대 (가장 높은 지대)
        color = vec3(0.95, 0.95, 0.95); // 흰색
    }

    // 최종 색상 출력
    FragColor = vec4(color, 1.0);
}