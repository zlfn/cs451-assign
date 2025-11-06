#include "base.hpp"
#include "utils.hpp"

// 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GLuint shaderProgram;
GLuint VAO;
GLuint VBO;

// 셰이더 소스 수정
const char *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;     // (x, y, 0) 그리드 위치
layout (location = 1) in float aHeight; // currentHeight[i][j].real 값
out float vHeight; // 프래그먼트 셰이더로 높이 값 전달

void main() {
    gl_Position = vec4(aPos, 1.0);
    
    // 높이(절대값)에 따라 점의 크기를 조절합니다.
    // 5.0f, 1.0f 등의 값은 시각적으로 보면서 조절하세요.
    gl_PointSize = (abs(aHeight) * 5.0f) + 1.0f; 
    
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

    // 간단한 'Jet' 컬러맵 (Blue -> Green -> Red)
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
    FragColor = vec4(heightToColor(vHeight), 1.0);
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

    // 셰이더에서 gl_PointSize를 사용하려면 이 옵션을 켜야 합니다.
    glEnable(GL_PROGRAM_POINT_SIZE);

    // --- 기존 삼각형 데이터 삭제 ---
    // float vertices[] = { ... };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // VBO를 동적으로 할당합니다.
    // GRID_SIZE*GRID_SIZE개의 정점, 각 정점은 (x, y, z, height) 4개의 float 값을 가짐.
    // (base.hpp 등에 GRID_SIZE, GRID_SIZE이 정의되어 있어야 합니다.)
    glBufferData(GL_ARRAY_BUFFER, GRID_SIZE * GRID_SIZE * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    // 1. layout (location = 0) : vec3 aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // 2. layout (location = 1) : float aHeight
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    // 삼각형(3개) 대신 GRID_SIZE*GRID_SIZE 개의 '점'을 그립니다.
    glDrawArrays(GL_POINTS, 0, GRID_SIZE * GRID_SIZE);

    glBindVertexArray(0);
    glUseProgram(0);

    glutSwapBuffers();
}

float currentTime = 0.0f;
// 정점 데이터를 담을 벡터 (전역으로 두거나 timer 내에서 매번 생성)
std::vector<float> vertices;

void timer(int value) {
    currentTime += TIMER_INTERVAL;
    calcWaveField(currentTime);
    iFFT();

    // currentHeight[i][j].real이 (i, j)에서의 파도 높이

    // --- VBO 업데이트 로직 ---
    vertices.clear();
    vertices.reserve(GRID_SIZE * GRID_SIZE * 4); // 메모리 재할당 방지

    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            // 그리드 좌표 (j, i)를 NDC 좌표 (-1 ~ 1)로 매핑
            float x = (j / (float)(GRID_SIZE - 1)) * 2.0f - 1.0f;
            float y = (i / (float)(GRID_SIZE - 1)) * 2.0f - 1.0f;
            float z = 0.0f; // 2D 시각화이므로 z=0
            float height = currentHeight[i][j].real() * 30;

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
    // --- VBO 업데이트 끝 ---

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
    glDeleteProgram(shaderProgram);
    std::cout << "Freed All Resources." << std::endl;
}

int main(int argc, char **argv) {
    glutInit(&argc, argv); // GLUT 초기화

    // 모던 opengl (Core Profile 3.3) 요청
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // 디스플레이 모드 설정 (더블 버퍼링, RGBA 색상)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("CSED451 Final Project");

    // GLEW 초기화
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW Initialization Failure: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
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
