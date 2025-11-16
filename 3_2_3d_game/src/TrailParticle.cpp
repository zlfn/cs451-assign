#include "base.hpp"
#include "graphics.hpp"

TrailParticle::TrailParticle(glm::fvec2 pos, glm::fvec2 vel, float sz, glm::fvec3 col,
                             int currentTime)
    : position(pos), velocity(vel), size(sz), alpha(0.8f), color(col), birthTime(currentTime) {}

bool TrailParticle::update(int currentTime, GameState &) {
    int deltaTime = currentTime - birthTime;
    float dt = static_cast<float>(deltaTime) * 0.001f;

    position += velocity * dt * 0.3f;
    alpha -= dt * 0.5f;
    size *= (1.0f - dt * 0.2f);

    return alpha <= 0.0f || size <= 0.001f || deltaTime > 2000;
}

// 빛나는 트레일 파티클 그리기
void TrailParticle::draw(const GameState &) {
    if (!g_shaderProgram) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    modelViewStack.matPush();
    modelViewStack.translate(position.x, position.y, 0.0f);
    modelViewStack.scale(size, size, 1.0f);

    // 중심이 밝은 원 그리기 (TRIANGLE_FAN을 TRIANGLES로 변환)
    const int N = 8;
    std::vector<float> vertices;
    vertices.reserve((N * 3) * 6); // N triangles, 3 vertices per triangle, 6 floats per vertex

    glm::vec3 centerColor(glm::min(color.r * 1.5f, 1.0f),
                          glm::min(color.g * 1.5f, 1.0f),
                          glm::min(color.b * 1.5f, 1.0f));
    glm::vec3 edgeColor(color.r * 0.3f, color.g * 0.3f, color.b * 0.3f);

    for (int i = 0; i < N; ++i) {
        float angle1 = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);
        float angle2 = static_cast<float>(i + 1) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);

        // 중심점
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(centerColor.r); vertices.push_back(centerColor.g); vertices.push_back(centerColor.b);

        // 첫 번째 가장자리 점
        vertices.push_back(std::cos(angle1)); vertices.push_back(std::sin(angle1)); vertices.push_back(0.0f);
        vertices.push_back(edgeColor.r); vertices.push_back(edgeColor.g); vertices.push_back(edgeColor.b);

        // 두 번째 가장자리 점
        vertices.push_back(std::cos(angle2)); vertices.push_back(std::sin(angle2)); vertices.push_back(0.0f);
        vertices.push_back(edgeColor.r); vertices.push_back(edgeColor.g); vertices.push_back(edgeColor.b);
    }

    Mesh mesh;
    mesh.setData(vertices.data(), vertices.size() * sizeof(float), GL_DYNAMIC_DRAW);
    mesh.setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    mesh.setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    mesh.setDrawMode(GL_TRIANGLES, N * 3);

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();

    drawMesh(mesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("objectColor", color);
        prog.setUniform("useVertexColor", 1.0f);
    });

    modelViewStack.matPop();
    glDisable(GL_BLEND);
}