#version 330 core
in vec3 aPos; // VBO/VAO를 통해 들어오는 정점 위치 (layout = 0)

// C++에서 glUniformMatrix4fv로 전달받을 행렬
uniform mat4 projection;
uniform mat4 modelview;

void main()
{
    // C++의 glFrustum/glOrtho * (glRotate/glScale/glTranslate) * vertex
    // 위 계산을 셰이더에서 직접 수행합니다.
    gl_Position = projection * modelview * vec4(aPos, 1.0);
}
