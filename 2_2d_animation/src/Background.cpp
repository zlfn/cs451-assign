#include "base.hpp"

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

void Star::draw(const GameState &) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(brightness, brightness, brightness, brightness * 0.8f);
    glPointSize(size);

    glPushMatrix();
    glTranslatef(position.x, position.y, -0.99f);

    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();

    glPopMatrix();
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

static void drawWorldBorder() {
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(5.0f);

    glBegin(GL_LINES);
    glVertex2f(-2.0f, -2.0f);
    glVertex2f(-2.0f, 2.0f);

    glVertex2f(-2.0f, 2.0f);
    glVertex2f(2.0f, 2.0f);

    glVertex2f(2.0f, 2.0f);
    glVertex2f(2.0f, -2.0f);

    glVertex2f(2.0f, -2.0f);
    glVertex2f(-2.0f, -2.0f);
    glEnd();
}

void Background::draw(const GameState &gameState) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);

    for (auto &star : stars) {
        star.draw(gameState);
    }

    drawWorldBorder();
}
