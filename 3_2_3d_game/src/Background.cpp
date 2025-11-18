#include "base.hpp"
#include "graphics.hpp"

Background::Background() : lastUpdateTime(0) {}

bool Background::update(int currentTime, GameState &gameState) {
    if (lastUpdateTime == 0) {
        lastUpdateTime = currentTime;
        return false;
    }

    lastUpdateTime = currentTime;
    return false;
}

// 경계선 그리기
static void drawWorldBorder() {
    if (!g_shaderProgram) return;

    glLineWidth(2.0f);

    // 정점 데이터 (position + color)
    float vertices[] = {
        // 아래쪽 사각형 (4 lines = 8 vertices)
        -2.0f, -2.0f, -1.0f,  1.0f, 1.0f, 1.0f,
        -2.0f, 2.0f, -1.0f,   1.0f, 1.0f, 1.0f,

        -2.0f, 2.0f, -1.0f,   1.0f, 1.0f, 1.0f,
        2.0f, 2.0f, -1.0f,    1.0f, 1.0f, 1.0f,

        2.0f, 2.0f, -1.0f,    1.0f, 1.0f, 1.0f,
        2.0f, -2.0f, -1.0f,   1.0f, 1.0f, 1.0f,

        2.0f, -2.0f, -1.0f,   1.0f, 1.0f, 1.0f,
        -2.0f, -2.0f, -1.0f,  1.0f, 1.0f, 1.0f,

        // 위쪽 사각형 (4 lines = 8 vertices)
        -2.0f, -2.0f, 1.0f,   1.0f, 1.0f, 1.0f,
        -2.0f, 2.0f, 1.0f,    1.0f, 1.0f, 1.0f,

        -2.0f, 2.0f, 1.0f,    1.0f, 1.0f, 1.0f,
        2.0f, 2.0f, 1.0f,     1.0f, 1.0f, 1.0f,

        2.0f, 2.0f, 1.0f,     1.0f, 1.0f, 1.0f,
        2.0f, -2.0f, 1.0f,    1.0f, 1.0f, 1.0f,

        2.0f, -2.0f, 1.0f,    1.0f, 1.0f, 1.0f,
        -2.0f, -2.0f, 1.0f,   1.0f, 1.0f, 1.0f,

        // 사각형 연결 (4 lines = 8 vertices)
        -2.0f, -2.0f, 1.0f,   1.0f, 1.0f, 1.0f,
        -2.0f, -2.0f, -1.0f,  1.0f, 1.0f, 1.0f,

        -2.0f, 2.0f, 1.0f,    1.0f, 1.0f, 1.0f,
        -2.0f, 2.0f, -1.0f,   1.0f, 1.0f, 1.0f,

        2.0f, 2.0f, 1.0f,     1.0f, 1.0f, 1.0f,
        2.0f, 2.0f, -1.0f,    1.0f, 1.0f, 1.0f,

        2.0f, -2.0f, 1.0f,    1.0f, 1.0f, 1.0f,
        2.0f, -2.0f, -1.0f,   1.0f, 1.0f, 1.0f
    };

    Mesh mesh;
    mesh.setData(vertices, sizeof(vertices), GL_STATIC_DRAW);
    mesh.setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    mesh.setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    mesh.setDrawMode(GL_LINES, 24); // 12 lines * 2 vertices

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();

    drawMesh(mesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
        prog.setUniform("useVertexColor", 1.0f);
    });
}

void Background::draw(const GameState &gameState) {
    drawWorldBorder();
}
