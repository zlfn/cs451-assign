#version 430 core

// 상수화된 조명 및 재질 값
#define LIGHT_POS_VS     vec3(0.0, 5.0, 0.0)
#define AMBIENT_COLOR    vec3(0.1, 0.1, 0.1)
#define LIGHT_COLOR      vec3(1.0, 1.0, 1.0)
#define SHININESS        32.0

uniform sampler2D colorSampler;  // texture map

in vec3 vertPosition_VS;
in vec3 vertNormal_VS;
in vec3 vertColor;

out vec4 finalColor;

void main() {
    vec3 normal_VS = normalize(vertNormal_VS);

    // 광원 벡터
    vec3 lightVec_VS = normalize(LIGHT_POS_VS - vertPosition_VS);
    vec3 viewVec_VS = normalize(-vertPosition_VS);

    // ambient
    vec3 ambient = AMBIENT_COLOR * vertColor;

    // diffuse
    float diff = max(dot(normal_VS, lightVec_VS), 0.0);
    vec3 diffuse = LIGHT_COLOR * diff * vertColor; 

    // specular
    vec3 reflectVec_VS = reflect(-lightVec_VS, normal_VS);
    float spec = pow(max(dot(viewVec_VS, reflectVec_VS), 0.0), SHININESS);
    vec3 specular = LIGHT_COLOR * spec;
    
    vec3 finalShadedColor = ambient + diffuse + specular;

    finalColor = vec4(finalShadedColor, 1.0);
}