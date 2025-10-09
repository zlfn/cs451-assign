#include "base.hpp"

EnemyBullet::EnemyBullet(glm::fvec2 initialDirection, glm::fvec2 initialPosition, float speed,
                         int initialTime, std::function<float(int, float)> posFunc)
    : initialDirection(glm::normalize(initialDirection) * speed),
      normalDirection(glm::normalize(glm::fvec2(-initialDirection.y, initialDirection.x))),
      initialPosition(initialPosition), currentPosition(initialPosition),
      previousPosition(initialPosition), initialTime(initialTime), lastTrailTime(initialTime),
      speed(speed), posFunc(std::move(posFunc)) {}

bool EnemyBullet::update(int currentTime, GameState &gameState) {
    previousPosition = currentPosition;
    int dt = currentTime - initialTime;
    currentPosition =
        initialPosition + float(dt) * initialDirection + posFunc(dt, speed) * normalDirection;

    // Create trail particles
    if (currentTime - lastTrailTime > 15) { // Create particles every 15ms (more frequent)
        lastTrailTime = currentTime;

        // Create 2 particles per update for denser trail
        for (int i = 0; i < 2; ++i) {
            // Add more spread to the trail
            std::uniform_real_distribution<float> offsetDist(-0.015f, 0.015f);
            std::uniform_real_distribution<float> velDist(-0.025f, 0.025f);

            glm::fvec2 trailPos = currentPosition + glm::fvec2(offsetDist(gen), offsetDist(gen));
            glm::fvec2 trailVel(velDist(gen), velDist(gen));
            float trailSize = 0.025f + offsetDist(gen) * 0.3f; // Bigger particles

            // Slightly dimmed purple/pink trail color for better contrast
            glm::fvec3 trailColor(0.6f, 0.25f, 0.8f);

            gameState.trailParticles.emplace_back(trailPos, trailVel, trailSize, trailColor,
                                                  currentTime);
        }
    }

    // Don't damage player if boss is already dying
    if (!gameState.bossObject1.isDying && !gameState.playerObject.isInvincible &&
        detectCollision(*this, gameState.playerObject)) {
        gameState.playerHealth -= 1;
        gameState.playerObject.takeDamage(currentTime);
        startCameraShake(currentTime);
        if (gameState.playerHealth < 0)
            gameState.playerHealth = 0;
        return true;
    }
    return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
}
void EnemyBullet::draw(const GameState &gameState) {
    const float BASE_SCALE = 0.03f;

    const float T_MS = static_cast<float>(glutGet(GLUT_ELAPSED_TIME));
    const float SPOKES_ROTATION_DEG = T_MS * 0.005f * 180.0f / std::numbers::pi_v<float>;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(currentPosition.x, currentPosition.y, 0.0f);
    glScalef(BASE_SCALE, BASE_SCALE, 1.0f);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.8f, 0.2f, 1.0f, 0.3f);
    glVertex3f(0.0f, 0.0f, -0.05f);
    glColor4f(0.4f, 0.0f, 0.8f, 0.0f);
    const int N = 12;
    const float OUTER_R = 2.0f;
    for (int i = 0; i <= N; ++i) {
        float ang = (float)i * glm::two_pi<float>() / (float)N;
        glVertex3f(OUTER_R * std::cos(ang), OUTER_R * std::sin(ang), -0.05f);
    }
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.3f, 0.8f, 1.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.7f, 0.0f, 0.0f);
    glVertex3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-0.7f, 0.0f, 0.0f);
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 0.5f, 0.01f);
    glVertex3f(0.35f, 0.0f, 0.01f);
    glVertex3f(0.0f, -0.5f, 0.01f);
    glVertex3f(-0.35f, 0.0f, 0.01f);
    glEnd();

    glPushMatrix();
    glRotatef(SPOKES_ROTATION_DEG, 0.0f, 0.0f, 1.0f);

    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor4f(0.6f, 0.1f, 1.0f, 0.7f);
    const float SPOKES_R = 1.2f;
    for (int i = 0; i < 4; ++i) {
        float ang = (float)i * glm::half_pi<float>();
        glVertex3f(0.0f, 0.0f, 0.02f);
        glVertex3f(SPOKES_R * std::cos(ang), SPOKES_R * std::sin(ang), 0.02f);
    }
    glEnd();

    glPopMatrix();
    glPopMatrix();

    glDisable(GL_BLEND);
}

CollisionShape EnemyBullet::getShape() const { return CollisionCircle(currentPosition, 0.03f); }

PlayerBullet::PlayerBullet(glm::fvec2 initialPosition, float speed, int initialTime)
    : initialPosition(initialPosition), currentPosition(initialPosition), initialTime(initialTime),
      speed(speed) {}

bool PlayerBullet::update(int currentTime, GameState &gameState) {
    currentPosition =
        initialPosition + glm::fvec2(0, speed * static_cast<float>(currentTime - initialTime));
    // Don't damage boss if player is already dying
    if (!gameState.playerObject.isDying && (detectCollision(*this, gameState.bossObject1) ||
                                            detectCollision(*this, gameState.bossObject2))) {
        gameState.bossHealth -= 1;
        if (gameState.bossHealth < 0)
            gameState.bossHealth = 0;
        return true;
    }
    return abs(currentPosition.x) > 1.0f || abs(currentPosition.y) > 1.0f;
}
void PlayerBullet::draw(const GameState &gameState) {
    const float WIDTH = 0.015f;
    const float HEIGHT = 0.04f;
    const float Z_DEPTH = 0.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(currentPosition.x, currentPosition.y, 0.0f);
    glRotatef(0.0f, 0.0f, 0.0f, 1.0f);
    glScalef(WIDTH, HEIGHT, 1.0f);

    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.9f, 0.0f, 1.0f);
    glVertex3f(0.0f, 1.0f, Z_DEPTH);
    glColor4f(1.0f, 0.5f, 0.0f, 0.8f);
    glVertex3f(-1.0f, -0.3f, Z_DEPTH);
    glVertex3f(1.0f, -0.3f, Z_DEPTH);
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.7f, 0.0f, 1.0f);
    glVertex3f(-0.6f, -0.2f, Z_DEPTH);
    glVertex3f(0.6f, -0.2f, Z_DEPTH);
    glColor4f(1.0f, 0.3f, 0.0f, 0.2f);
    glVertex3f(0.4f, -1.5f, Z_DEPTH);
    glVertex3f(-0.4f, -1.5f, Z_DEPTH);
    glEnd();

    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glColor4f(1.0f, 1.0f, 0.7f, 0.9f);
    glVertex3f(0.0f, 0.7f, Z_DEPTH);
    glEnd();

    glPopMatrix();

    glDisable(GL_BLEND);
}
CollisionShape PlayerBullet::getShape() const {
    return CollisionRectangle(currentPosition - glm::fvec2(0.015f, 0.015f),
                              currentPosition + glm::fvec2(0.015f, 0.015f));
}
