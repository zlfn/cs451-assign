#include "base.hpp"
#include "utils.hpp"

// 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GLuint shaderProgram;
GLuint VAO;
GLuint VBO;
GLuint EBO;

// 셰이더 소스 수정
const char *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aHeight;
out float vHeight;

void main() {
    // Scale height to make waves visible
    vec3 pos = vec3(aPos.xy, aHeight * 5.0f);

    // Apply a static rotation around the X-axis for a better viewing angle
    float angle = -1.2f; // ~70 degrees
    mat4 rotation = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, cos(angle), -sin(angle), 0.0,
        0.0, sin(angle), cos(angle), 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    gl_Position = rotation * vec4(pos, 1.0);
    
    vHeight = aHeight;
}
)";

const char *fragmentShaderSource = R"(
#version 330 core

in float vHeight; // 정점 셰이더에서 전달받은 높이
out vec4 FragColor;

// 높이 값(t)을 색상(Red-Green-Blue)으로 매핑하는 함수
vec3 heightToColor(float t) {
    // 높이 값을 -1 ~ 1 범위에서 0 ~ 1 범위로 정규화 (범위는 데이터에 맞게 조절)
    float value = (t * 0.5) + 0.5;

    // 간단한 Jet 컬러맵 (Blue -> Green -> Red)
    vec3 low = vec3(0.0, 0.0, 1.0);  // Blue (낮음)
    vec3 mid = vec3(0.0, 1.0, 0.0);  // Green (중간)
    vec3 high = vec3(1.0, 0.0, 0.0); // Red (높음)
    
    vec3 color = mix(low, mid, value * 2.0);
    if (value > 0.5) {
        color = mix(mid, high, (value - 0.5) * 2.0);
    }
    return color;
}

void main() {
    // 높이 값에 따라 색상 결정
    FragColor = vec4(heightToColor(vHeight*100), 1.0);
}
)";

// 셰이더 컴파일
GLuint createShaderProgram() {
    GLint success;
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();

    // 정점 셰이더 컴파일
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success); // 오류 검사
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Compile Failure:\n" << infoLog << std::endl;
    }

    // 프래그먼트 셰이더 컴파일
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success); // 오류 검사
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Compile Failure:\n" << infoLog << std::endl;
    }

    // 셰이더 프로그램 링크
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success); // 링크 오류 검사
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader Program Link Failure:\n" << infoLog << std::endl;
    }

    // 컴파일 & 링크 후 개별 shader 객체는 삭제
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void init() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    shaderProgram = createShaderProgram();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, GRID_SIZE * GRID_SIZE * 4 * sizeof(float), NULL,
                 GL_DYNAMIC_DRAW);

    // Vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create and bind EBO with indices for a grid mesh
    std::vector<unsigned int> indices;
    indices.reserve((GRID_SIZE - 1) * (GRID_SIZE - 1) * 6);
    for (int i = 0; i < GRID_SIZE - 1; ++i) {
        for (int j = 0; j < GRID_SIZE - 1; ++j) {
            unsigned int v0 = i * GRID_SIZE + j;
            unsigned int v1 = i * GRID_SIZE + j + 1;
            unsigned int v2 = (i + 1) * GRID_SIZE + j;
            unsigned int v3 = (i + 1) * GRID_SIZE + j + 1;
            // Triangle 1
            indices.push_back(v0);
            indices.push_back(v2);
            indices.push_back(v1);
            // Triangle 2
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
        }
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    // Draw wireframe mesh
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, (GRID_SIZE - 1) * (GRID_SIZE - 1) * 6, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset for other potential draws

    glBindVertexArray(0);
    glUseProgram(0);

    glutSwapBuffers();
}

float currentTime = 0.0f;
float timeScale = 0.0005f;
// 정점 데이터를 담을 벡터 (전역으로 두거나 timer 내에서 매번 생성)
std::vector<float> vertices;

void timer(int value) {
    currentTime += TIMER_INTERVAL * timeScale;
    calcWaveField(currentTime);
    iFFT();

    // currentHeight[i][j].real이 (i, j)에서의 파도 높이

    // VBO 업데이트 로직
    vertices.clear();
    vertices.reserve(GRID_SIZE * GRID_SIZE * 4); // 메모리 재할당 방지

    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            // 그리드 좌표 (j, i)를 NDC 좌표 (-1 ~ 1)로 매핑
            float x = (j / (float)(GRID_SIZE - 1)) * 2.0f - 1.0f;
            float y = (i / (float)(GRID_SIZE - 1)) * 2.0f - 1.0f;
            float z = 0.0f; // 2D 시각화이므로 z=0
            float height = currentHeight[i][j].real();

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(height); // 높이 값을 추가 속성으로 전달
        }
    }

    // VBO 데이터 업데이트
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glutPostRedisplay(); // 화면 다시 그리기
    glutTimerFunc(TIMER_INTERVAL, timer, value);
}

void reshape(int width, int height) { glViewport(0, 0, width, height); }

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // ESC 키
        glutLeaveMainLoop(); // 메인 루프 종료
    }
}

// 리소스 초기화
void cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    std::cout << "Freed All Resources." << std::endl;
}

int main(int argc, char **argv) {
    glutInit(&argc, argv); // GLUT 초기화

    // 모던 opengl (Core Profile 3.3) 요청
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // 디스플레이 모드 설정 (더블 버퍼링, RGBA 색상, 깊이 버퍼)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("CSED451 Final Project");

    // GLEW 초기화
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW Initialization Failure: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST); // 3D 렌더링을 위한 깊이 테스트 활성화
    std::cout << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

    init(); // 셰이더, VBO/VAO 생성

    // GLUT 콜백 함수
    glutDisplayFunc(display); // 렌더링 콜백
    glutTimerFunc(TIMER_INTERVAL, timer, 0);
    glutReshapeFunc(reshape); // 창 크기 조절 콜백
    glutKeyboardFunc(keyboard); // 키보드 콜백
    glutCloseFunc(cleanup); // 창 닫을 때 cleanup 호출

    initSpectra(); // spectrum 초기화

    // 메인 루프 시작
    glutMainLoop();

    return 0;
}
