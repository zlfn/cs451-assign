#include "base.hpp"

BossIndicator::BossIndicator() {}

void BossIndicator::drawArrow(float x, float y, float angle, float size, glm::fvec4 color) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.4f);
    glVertex3f(0.0f, 0.0f, 0.95f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    for (int i = 0; i <= 12; ++i) {
        float ang = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 12.0f;
        glVertex3f(std::cos(ang) * size * 2.0f, std::sin(ang) * size * 2.0f, 0.95f);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, color.a * 0.6f);
    glVertex3f(0.0f, 0.0f, 0.96f);
    glColor4f(color.r, color.g, color.b, color.a * 0.3f);
    for (int i = 0; i <= 12; ++i) {
        float ang = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 12.0f;
        glVertex3f(std::cos(ang) * size * 1.5f, std::sin(ang) * size * 1.5f, 0.96f);
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 1.0f, 1.0f, color.a);
    glVertex3f(0.0f, size * 1.2f, 0.97f);
    glColor4f(color.r * 1.2f, color.g * 1.2f, color.b * 1.2f, color.a);
    glVertex3f(-size * 0.6f, -size * 0.6f, 0.97f);
    glVertex3f(size * 0.6f, -size * 0.6f, 0.97f);
    glEnd();

    glLineWidth(3.5f);
    glBegin(GL_LINE_LOOP);
    glColor4f(0.0f, 0.0f, 0.0f, color.a * 0.9f);
    glVertex3f(0.0f, size * 1.2f, 0.98f);
    glVertex3f(-size * 0.6f, -size * 0.6f, 0.98f);
    glVertex3f(size * 0.6f, -size * 0.6f, 0.98f);
    glEnd();

    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glColor4f(1.0f, 1.0f, 1.0f, color.a);
    glVertex3f(0.0f, size * 1.2f, 0.99f);
    glVertex3f(-size * 0.6f, -size * 0.6f, 0.99f);
    glVertex3f(size * 0.6f, -size * 0.6f, 0.99f);
    glEnd();

    glDisable(GL_BLEND);
    glPopMatrix();
}

void BossIndicator::draw(const GameState &gameState) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(gameState.cameraBaseOffset.x, gameState.cameraBaseOffset.y, 0);

    const float SCREEN_LEFT = -0.95f;
    const float SCREEN_RIGHT = 0.95f;
    const float SCREEN_BOTTOM = -0.95f;
    const float SCREEN_TOP = 0.88f;
    const float MARGIN = 0.12f;

    std::vector<const Boss *> bosses = {&gameState.bossObject1, &gameState.bossObject2};

    for (const Boss *boss : bosses) {
        if (boss->isDying) {
            continue;
        }

        glm::fvec2 bossPos = boss->currentPosition;
        glm::fvec2 bossScreenPos = bossPos - gameState.cameraBaseOffset;

        bool isOffScreen = (bossScreenPos.x < SCREEN_LEFT || bossScreenPos.x > SCREEN_RIGHT ||
                            bossScreenPos.y < SCREEN_BOTTOM || bossScreenPos.y > SCREEN_TOP);

        if (isOffScreen) {
            float arrowX = bossScreenPos.x;
            float arrowY = bossScreenPos.y;

            if (bossScreenPos.x < SCREEN_LEFT) {
                arrowX = SCREEN_LEFT + MARGIN;
            } else if (bossScreenPos.x > SCREEN_RIGHT) {
                arrowX = SCREEN_RIGHT - MARGIN;
            }

            if (bossScreenPos.y < SCREEN_BOTTOM) {
                arrowY = SCREEN_BOTTOM + MARGIN;
            } else if (bossScreenPos.y > SCREEN_TOP) {
                arrowY = SCREEN_TOP - MARGIN;
            }

            glm::fvec2 direction = glm::normalize(bossScreenPos - glm::fvec2(arrowX, arrowY));
            float angle =
                std::atan2(direction.y, direction.x) * 180.0f / std::numbers::pi_v<float> - 90.0f;

            float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;
            float pulse = 0.7f + 0.3f * std::sin(time * 3.0f);

            glm::fvec4 color;
            if (boss->bossId == 1) {
                color = glm::fvec4(0.8f, 0.2f, 1.0f, pulse);
            } else {
                color = glm::fvec4(0.3f, 0.5f, 1.0f, pulse);
            }

            drawArrow(arrowX, arrowY, angle, 0.035f, color);
        }
    }

    glPopMatrix();
}
