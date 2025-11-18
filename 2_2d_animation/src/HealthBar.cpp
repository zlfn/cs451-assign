#include "base.hpp"

PlayerHealthBar::PlayerHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
// 플레이어 체력 바 그리기
void PlayerHealthBar::draw(const GameState &gameState) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(gameState.cameraBaseOffset.x, gameState.cameraBaseOffset.y, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int maxHealth = gameState.MAX_PLAYER_HEALTH;
    int currentHealth = gameState.playerHealth;

    // 세그먼트 크기 계산
    float maxTotalWidth = 1.0f;
    float spacing = 0.015f;
    float totalSpacing = spacing * static_cast<float>(maxHealth - 1);
    float availableWidth = maxTotalWidth - totalSpacing - 0.05f;
    float segmentWidth = availableWidth / static_cast<float>(maxHealth);

    segmentWidth = glm::min(segmentWidth, 0.2f);

    // 화면 왼쪽 하단에 위치
    float startX = -0.975f;
    float startY = -0.95f;
    float segmentHeight = 0.04f;
    float zDepth = 0.9f;

    // 각 체력 포인트를 세그먼트로 그림
    for (int i = 0; i < maxHealth; i++) {
        float x = startX + static_cast<float>(i) * (segmentWidth + spacing) + segmentWidth / 2.0f;

        glm::fvec4 color;
        if (i < currentHealth) {
            // 활성 체력 - 밝은 주황색
            float intensity =
                0.8f + 0.2f * std::sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.003f +
                                       static_cast<float>(i) * 0.5f);
            color = glm::fvec4(1.0f, 0.5f * intensity, 0.1f, 0.9f);
        } else {
            // 잃은 체력 - 어두운 회색
            color = glm::fvec4(0.2f, 0.2f, 0.2f, 0.5f);
        }

        drawRectWithGlow(x, startY, segmentWidth, segmentHeight, color,
                         (i < currentHealth) ? 0.02f : 0.0f, zDepth);
    }

    glDisable(GL_BLEND);
    glPopMatrix();
}

BossHealthBar::BossHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}

// 보스 체력 바 그리기
void BossHealthBar::draw(const GameState &gameState) {
    float healthPercentage =
        static_cast<float>(gameState.bossHealth) / static_cast<float>(gameState.MAX_BOSS_HEALTH);
    healthPercentage = glm::clamp(healthPercentage, 0.0f, 1.0f);

    float barWidth = 1.9f;
    float barHeight = 0.03f;
    float barX = 0.0f;
    float barY = 0.95f;
    float zDepth = 0.9f;

    glPushMatrix();
    glLoadIdentity();
    glTranslatef(gameState.cameraBaseOffset.x, gameState.cameraBaseOffset.y, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 배경 바 그리기
    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.05f, 0.2f, 0.8f);
    glVertex3f(-barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY + barHeight / 2, zDepth);
    glVertex3f(-barWidth / 2, barY + barHeight / 2, zDepth);
    glEnd();

    // 체력 바 그리기
    float healthBarWidth = barWidth * healthPercentage;
    float healthBarX = -barWidth / 2 + healthBarWidth / 2;

    glm::fvec4 healthColor;

    if (healthPercentage > 0.5f) {
        healthColor = glm::fvec4(0.6f, 0.2f, 1.0f, 1.0f);
    } else if (healthPercentage > 0.25f) {
        healthColor = glm::fvec4(0.8f, 0.3f, 0.8f, 1.0f);
    } else {
        healthColor = glm::fvec4(1.0f, 0.2f, 0.6f, 1.0f);
    }

    if (gameState.bossHealth > 0) {
        drawRectWithGlow(healthBarX, barY, healthBarWidth, barHeight, healthColor, 0.05f, 1.0f);
    }

    glDisable(GL_BLEND);

    glPopMatrix();
}
