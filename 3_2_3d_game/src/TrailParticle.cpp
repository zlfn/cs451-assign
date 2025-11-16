#include "base.hpp"

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    modelViewStack.matPush();
    modelViewStack.translate(position.x, position.y, 0.0f);
    modelViewStack.scale(size, size, 1.0f);

    // 중심이 밝은 원 그리기
    glBegin(GL_TRIANGLE_FAN);
    // 밝은 중심
    glColor4f(glm::min(color.r * 1.5f, 1.0f), glm::min(color.g * 1.5f, 1.0f),
              glm::min(color.b * 1.5f, 1.0f), alpha * 1.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    // 어두운 가장자리
    glColor4f(color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, 0.0f);
    const int N = 8;
    for (int i = 0; i <= N; ++i) {
        float angle =
            static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);
        glVertex3f(std::cos(angle), std::sin(angle), 0.0f);
    }
    glEnd();

    modelViewStack.matPop();
    glDisable(GL_BLEND);
}