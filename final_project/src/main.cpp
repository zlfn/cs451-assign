#include "utils.hpp"
#include "base.hpp"

// 기존 전역 변수
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);
bool keyStates[256] = {false};

// 모던 OpenGL을 위한 전역 변수
GLuint shaderProgram; // 3D와 2D가 동일한 셰이더를 공유
GLuint objectVAO;     // 3D 객체 (예: 삼각형)
GLuint uiVAO;         // 2D UI (예: 사각형)
GLuint objectVBO, uiVBO;

// Uniform 변수의 위치(location)를 저장할 전역 변수
GLint projLoc;
GLint mvLoc;
GLint colorLoc;

// 셰이더 로딩 헬퍼 함수

// 셰이더 파일 로드
std::string loadShaderSource(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "셰이더 파일을 열 수 없습니다: " << filepath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 셰이더 컴파일 및 링크
GLuint createShaderProgram(const std::string &vertPath, const std::string &fragPath) {
    std::string vertSource = loadShaderSource(vertPath);
    std::string fragSource = loadShaderSource(fragPath);
    const char *vs = vertSource.c_str();
    const char *fs = fragSource.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vs, NULL);
    glCompileShader(vertexShader);
    // (*** 컴파일 오류 체크 추가 권장 ***)

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fs, NULL);
    glCompileShader(fragmentShader);
    // (*** 컴파일 오류 체크 추가 권장 ***)

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    // (*** 링크 오류 체크 추가 권장 ***)

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// 지오메트리 초기화
void initGeometry() {
    // 3D 객체 (삼각형) - TODO: object draw 대체
    float objectVertices[] = {0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

    glGenVertexArrays(1, &objectVAO);
    glGenBuffers(1, &objectVBO);
    glBindVertexArray(objectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, objectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(objectVertices), objectVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0); // layout 0 (aPos)
    glEnableVertexAttribArray(0);

    // 2D UI (사각형) - TODO: ui 추가하기 대체
    float uiVertices[] = {// (x, y, z) - 2D지만 z는 0
                          -0.5f, -0.5f, 0.0f, 0.5f,  -0.5f, 0.0f, 0.5f,  0.5f,  0.0f,
                          0.5f,  0.5f,  0.0f, -0.5f, 0.5f,  0.0f, -0.5f, -0.5f, 0.0f};

    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), uiVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0); // layout 0 (aPos)
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// OpenGL 초기화
void init() {
    // 셰이더 프로그램 생성
    shaderProgram = createShaderProgram("shader.vert", "shader.frag");
    if (shaderProgram == 0) {
        std::cerr << "셰이더 프로그램 생성 실패!" << std::endl;
        std::exit(1);
    }

    // Uniform 변수 위치 미리 찾아두기 (성능 최적화)
    // 셰이더 프로그램을 한 번만 활성화하고 위치를 가져옴
    glUseProgram(shaderProgram);
    projLoc = glGetUniformLocation(shaderProgram, "projection");
    mvLoc = glGetUniformLocation(shaderProgram, "modelview");
    colorLoc = glGetUniformLocation(shaderProgram, "ourColor");
    glUseProgram(0); // 다시 비활성화

    // VBO/VAO 생성
    initGeometry();

    // 기존 깊이 테스트 활성화
    glEnable(GL_DEPTH_TEST);
}

// 리소스 해제
void cleanup() {
    glDeleteVertexArrays(1, &objectVAO);
    glDeleteVertexArrays(1, &uiVAO);
    glDeleteBuffers(1, &objectVBO);
    glDeleteBuffers(1, &uiVBO);
    glDeleteProgram(shaderProgram);
    std::cout << "자원 해제 완료." << std::endl;
}

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = false; }

void keyInputUpdate(int dt) {
    if (keyStates[27]) {
        std::cout << "ESC pressed -> exit\n";
        glutLeaveMainLoop(); // 안전한 종료
    }
    bool movingHorizontal = false;
}

void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    static int lastMs = now;
    int dt = now - lastMs;
    lastMs = now;

    keyInputUpdate(dt);
    
    // TOOD: update

    glutTimerFunc(16, timer, 0);
}

void reshape(int width, int height) {
    if (width != 800 || height != 800) {
        glutReshapeWindow(800, 800);
    }
    glViewport(0, 0, 800, 800);
}

void display() {
    // 설정 값 (원본과 동일)
    const float SCALE = 3.0f;
    const float CAMERA_ANGLE_X_DEG = 50.0f;
    const float Z_DIST_VIEW = -0.5f;
    const float ANGLE_RAD = (float)(CAMERA_ANGLE_X_DEG * (std::numbers::pi / 180.0f));
    const float Y_COMPENSATION = (std::tan(ANGLE_RAD) * std::abs(Z_DIST_VIEW)) / SCALE;
    const float Z_TRANSLATE = Z_DIST_VIEW / std::cos(ANGLE_RAD) / SCALE;
    glm::fvec2 cbo = glm::fvec2(0.0f, 0.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 셰이더 프로그램 활성화
    glUseProgram(shaderProgram);

    // --- 1. 3D 투영 및 렌더링 ---
    // glMatrixMode(GL_PROJECTION) + glFrustum(...)
    glm::mat4 projection_3d = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.5f, 20.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_3d));

    // 스카이박스 그리기 (회전만 적용)
    // glMatrixMode(GL_MODELVIEW) + glLoadIdentity() + glRotatef(...)
    glm::mat4 modelview_skybox = glm::mat4(1.0f); // 단위 행렬
    modelview_skybox = glm::rotate(modelview_skybox, glm::radians(-CAMERA_ANGLE_X_DEG),
                                   glm::vec3(1.0f, 0.0f, 0.0f));

    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(modelview_skybox));
    glUniform4f(colorLoc, 0.5f, 0.5f, 0.8f, 1.0f); // 하늘색

    // (실제 스카이박스 VAO를 사용해야 하지만, 여기선 objectVAO로 대체)
    // glBindVertexArray(skyboxVAO);
    // glDrawArrays(...);

    // 다른 객체 그리기 (모든 변환 적용)
    // glRotatef + glScalef + glTranslatef
    glm::mat4 modelview_objects = glm::mat4(1.0f); // 단위 행렬
    modelview_objects = glm::rotate(modelview_objects, glm::radians(-CAMERA_ANGLE_X_DEG),
                                    glm::vec3(1.0f, 0.0f, 0.0f));
    modelview_objects = glm::scale(modelview_objects, glm::vec3(SCALE, SCALE, SCALE));
    modelview_objects =
        glm::translate(modelview_objects, glm::vec3(-cbo.x, -cbo.y + Y_COMPENSATION, Z_TRANSLATE));

    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(modelview_objects));
    glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f); // 빨간색 (3D 객체)

    // TODO: object draw (샘플 삼각형 그리기로 대체)
    glBindVertexArray(objectVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // --- 2. 2D 투영 및 렌더링 (UI) ---
    // 2D 렌더링 전 깊이 버퍼 테스트 비활성화 (선택적)
    // glDisable(GL_DEPTH_TEST);

    // glMatrixMode(GL_PROJECTION) + glOrtho(...)
    glm::mat4 projection_2d = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_2d));

    // glMatrixMode(GL_MODELVIEW) + glLoadIdentity()
    glm::mat4 modelview_ui = glm::mat4(1.0f);
    // (예: UI를 좌측 상단으로 이동 및 크기 조절)
    modelview_ui = glm::translate(modelview_ui, glm::vec3(-0.8f, 0.8f, 0.0f));
    modelview_ui = glm::scale(modelview_ui, glm::vec3(0.2f, 0.2f, 1.0f));

    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(modelview_ui));
    glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f); // 초록색 (UI)

    // TODO: ui 추가하기 (샘플 사각형 그리기로 대체)
    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6); // 사각형은 6개의 정점

    // 3D 렌더링을 위해 깊이 테스트 다시 활성화
    // glEnable(GL_DEPTH_TEST);

    // 정리
    glBindVertexArray(0); // VAO 바인딩 해제
    glUseProgram(0);      // 셰이더 프로그램 비활성화

    glutSwapBuffers();
    glutPostRedisplay();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);

    // 모던 OpenGL (Core Profile 3.3) 요청
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("CSED451 Final Project");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << '\n';
        return -1;
    }
    std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

    // opengl 초기화
    init();

    // 콜백 등록
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);
    glutCloseFunc(cleanup);

    glutMainLoop();
    return 0;
}
