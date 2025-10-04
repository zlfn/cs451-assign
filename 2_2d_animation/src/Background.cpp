#include "base.hpp"

Star::Star(glm::fvec2 pos, float spd, float sz, float br)
    : position(pos), speed(spd), size(sz), brightness(br) {}

bool Star::update(int deltaTime, GameState & /*gameState*/) {
    position.y -= speed * static_cast<float>(deltaTime);
    if (position.y < -1.1f) {
        position.y = 1.1f;
        position.x = -1.0f + dist(gen) * 2.0f;
    }
    return false;
}

void Star::draw(glm::fvec2 cameraOffset, const GameState &) {
    const glm::fvec2 VIEW_POS = position - cameraOffset;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(brightness, brightness, brightness, brightness * 0.8f);
    glPointSize(size);

    glPushMatrix();
    glTranslatef(VIEW_POS.x, VIEW_POS.y, -0.99f);

    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();

    glPopMatrix();
    glDisable(GL_BLEND);
}

Background::Background() : lastUpdateTime(0) { initializeStars(); }

void Background::initializeStars() {
    const int NUM_STARS = 60;
    stars.reserve(NUM_STARS);
    for (int i = 0; i < NUM_STARS; ++i) {
        float x = -1.0f + dist(gen) * 2.0f;
        float y = -1.0f + dist(gen) * 2.0f;
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

void Background::draw(glm::fvec2 cameraOffset, const GameState &gameState) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);

    for (auto &star : stars) {
        star.draw(cameraOffset, gameState);
    }

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
}
