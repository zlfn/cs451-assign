#include "base.hpp"
#include "graphics.hpp"

Star::Star(glm::fvec2 pos, float spd, float sz, float br)
    : position(pos), speed(spd), size(sz), brightness(br) {}

bool Star::update(int deltaTime, GameState & /*gameState*/) {
    position.y -= speed * static_cast<float>(deltaTime);
    if (position.y < -2.2f) {
        position.y = 2.2f;
        position.x = -2.0f + dist(gen) * 4.0f;
    }
    return false;
}

void Star::draw(const GameState &gameState) {
    if (!g_shaderProgram) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(size);

    modelViewStack.matPush();
    modelViewStack.translate(position.x, position.y, -0.99f);

    // 정점 데이터 (position + color)
    float vertices[] = {
        0.0f, 0.0f, 0.0f,  // position
        brightness, brightness, brightness  // color
    };

    Mesh mesh;
    mesh.setData(vertices, sizeof(vertices), GL_DYNAMIC_DRAW);
    mesh.setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    mesh.setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    mesh.setDrawMode(GL_POINTS, 1);

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();

    drawMesh(mesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("objectColor", glm::vec3(brightness, brightness, brightness));
        prog.setUniform("useVertexColor", 1.0f);
    });

    modelViewStack.matPop();
    glDisable(GL_BLEND);
}

Background::Background() : lastUpdateTime(0) { initializeStars(); }

void Background::initializeStars() {
    const int NUM_STARS = 100;
    stars.reserve(NUM_STARS);
    for (int i = 0; i < NUM_STARS; ++i) {
        float x = -2.0f + dist(gen) * 4.0f;
        float y = -2.0f + dist(gen) * 4.0f;
        float speed = 0.0008f + dist(gen) * 0.001f;
        float size = 1.0f + dist(gen) * 3.0f;
        float brightness = 0.3f + dist(gen) * 0.7f;
        stars.emplace_back(glm::fvec2(x, y), speed, size, brightness);
    }
}

bool Background::update(int currentTime, GameState &gameState) {
    if (lastUpdateTime == 0) {
        lastUpdateTime = currentTime;
        return false;
    }

    int deltaTime = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;

    for (auto &star : stars) {
        star.update(deltaTime, gameState);
    }
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);

    // 별 그리기 비활성화 (성능 문제)
    // for (auto &star : stars) {
    //     star.draw(gameState);
    // }

    drawWorldBorder();
}
