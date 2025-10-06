#include "base.hpp"

TrailParticle::TrailParticle(glm::fvec2 pos, glm::fvec2 vel, float sz, glm::fvec3 col,
                             int currentTime)
    : position(pos), velocity(vel), size(sz), alpha(0.8f), color(col), birthTime(currentTime) {}

bool TrailParticle::update(int currentTime, GameState &) {
    int deltaTime = currentTime - birthTime;
    float dt = static_cast<float>(deltaTime) * 0.001f;

    // Slower drift and fade
    position += velocity * dt * 0.3f;
    alpha -= dt * 0.5f;
    size *= (1.0f - dt * 0.2f);

    return alpha <= 0.0f || size <= 0.001f || deltaTime > 2000; // Last up to 2 seconds
}

void TrailParticle::draw(glm::fvec2 cameraOffset, const GameState &) {
    const glm::fvec2 VIEW_POS = position - cameraOffset;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(VIEW_POS.x, VIEW_POS.y, -0.1f);
    glScalef(size, size, 1.0f);

    // Draw a glowing circle with brighter center
    glBegin(GL_TRIANGLE_FAN);
    // Much brighter center (almost white)
    glColor4f(glm::min(color.r * 1.5f, 1.0f), glm::min(color.g * 1.5f, 1.0f),
              glm::min(color.b * 1.5f, 1.0f), alpha * 1.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    // Fade to darker edges
    glColor4f(color.r * 0.3f, color.g * 0.3f, color.b * 0.3f, 0.0f);
    const int N = 8;
    for (int i = 0; i <= N; ++i) {
        float angle =
            static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(N);
        glVertex3f(std::cos(angle), std::sin(angle), 0.0f);
    }
    glEnd();

    glPopMatrix();
    glDisable(GL_BLEND);
}