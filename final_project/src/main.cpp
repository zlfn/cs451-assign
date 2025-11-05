#include "base.hpp"
#include "utils.hpp"

// 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GLuint shaderProgram; // 셰이더 프로그램 ID
GLuint VAO; // Vertex Array Object ID
GLuint VBO; // Vertex Buffer Object ID

// 정점 셰이더 (Vertex Shader): 정점 위치(aPos)를 변환 없이 그대로 내보냄
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

// 프래그먼트 셰이더 (Fragment Shader): 모든 픽셀을 흰색으로 칠함x
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
)";

// 셰이더 컴파일 헬퍼 함수
GLuint createShaderProgram() {
    // 정점 셰이더 컴파일
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success; // 오류 검사
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "정점 셰이더 컴파일 실패:\n" << infoLog << std::endl;
    }

    // 프래그먼트 셰이더 컴파일
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success); // 오류 검사
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "프래그먼트 셰이더 컴파일 실패:\n" << infoLog << std::endl;
    }

    // 셰이더 프로그램 링크
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success); // 링크 오류 검사
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "셰이더 프로그램 링크 실패:\n" << infoLog << std::endl;
    }

    // 컴파일/링크 후 개별 셰이더 객체는 삭제
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void init() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // 배경색

    // 셰이더 프로그램 생성
    shaderProgram = createShaderProgram();

    // 화면에 그릴 삼각형 정점 데이터
    float vertices[] = {
        0.0f,  0.5f,  0.0f, // 상단 중앙
        -0.5f, -0.5f, 0.0f, // 하단 좌측
        0.5f,  -0.5f, 0.0f  // 하단 우측
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);

    glutSwapBuffers();
}

void reshape(int width, int height) { glViewport(0, 0, width, height); }

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) // ESC 키
    {
        glutLeaveMainLoop(); // 메인 루프 종료
    }
}

// 리소스 초기화
void cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    std::cout << "리소스 해제 완료." << std::endl;
}

int main(int argc, char **argv) {
    // GLUT 초기화
    glutInit(&argc, argv);

    // 모던 OpenGL (Core Profile 3.3) 요청
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    // 디스플레이 모드 설정 (더블 버퍼링, RGBA 색상)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("CSED451 Final Project");

    // GLEW 초기화 (반드시 glutCreateWindow 후에)
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    std::cout << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

    // OpenGL 초기화 (셰이더, VBO/VAO 생성)
    init();

    // GLUT 콜백 함수
    glutDisplayFunc(display);   // 렌더링 콜백
    glutReshapeFunc(reshape);   // 창 크기 조절 콜백
    glutKeyboardFunc(keyboard); // 키보드 콜백
    glutCloseFunc(cleanup);     // 창 닫을 때 cleanup 호출

    // 메인 루프 시작
    glutMainLoop();

    return 0;
}
