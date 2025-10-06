#include "base.hpp"

PlayerHealthBar::PlayerHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
void PlayerHealthBar::draw(glm::fvec2 cameraOffset, const GameState &gameState) {
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int maxHealth = gameState.MAX_PLAYER_HEALTH;
    int currentHealth = gameState.playerHealth;

    // Calculate segment size based on max health to fit within half screen width
    float maxTotalWidth = 1.0f; // Half of screen width
    float spacing = 0.015f;
    float totalSpacing = spacing * static_cast<float>(maxHealth - 1);
    float availableWidth = maxTotalWidth - totalSpacing - 0.05f; // Leave small margin
    float segmentWidth = availableWidth / static_cast<float>(maxHealth);

    // Limit segment width to prevent too large segments
    segmentWidth = glm::min(segmentWidth, 0.2f);

    // Recalculate total width with actual segment width
    // totalWidth is calculated but not currently used

    // Position at bottom-left corner of screen
    float startX = -0.975f; // Near left edge
    float startY = -0.95f;  // Near bottom edge
    float segmentHeight = 0.04f;
    float zDepth = 0.9f;

    // Draw rectangle segments for each health point
    for (int i = 0; i < maxHealth; i++) {
        float x = startX + static_cast<float>(i) * (segmentWidth + spacing) + segmentWidth / 2.0f;

        glm::fvec4 color;
        if (i < currentHealth) {
            // Active health - bright orange with gradient
            float intensity =
                0.8f + 0.2f * std::sinf(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.003f +
                                        static_cast<float>(i) * 0.5f);
            color = glm::fvec4(1.0f, 0.5f * intensity, 0.1f, 0.9f);
        } else {
            // Lost health - dark gray
            color = glm::fvec4(0.2f, 0.2f, 0.2f, 0.5f);
        }

        // Draw the rectangle with glow
        drawRectWithGlow(x, startY, segmentWidth, segmentHeight, color,
                         (i < currentHealth) ? 0.02f : 0.0f, zDepth);
    }

    glDisable(GL_BLEND);
    glPopMatrix();
}

BossHealthBar::BossHealthBar(glm::fvec2 drawPosition) : drawPosition(drawPosition) {}
void BossHealthBar::draw(glm::fvec2 cameraOffset, const GameState &gameState) {
    float healthPercentage =
        static_cast<float>(gameState.bossHealth) / static_cast<float>(gameState.MAX_BOSS_HEALTH);
    healthPercentage = glm::clamp(healthPercentage, 0.0f, 1.0f);

    float barWidth = 1.9f; // Almost full screen width
    float barHeight = 0.03f;
    float barX = 0.0f;
    float barY = 0.95f;
    float zDepth = 0.9f;

    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw background bar with purple tint
    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.05f, 0.2f, 0.8f);
    glVertex3f(-barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY - barHeight / 2, zDepth);
    glVertex3f(barWidth / 2, barY + barHeight / 2, zDepth);
    glVertex3f(-barWidth / 2, barY + barHeight / 2, zDepth);
    glEnd();

    // Draw health bar with purple gradient
    float healthBarWidth = barWidth * healthPercentage;
    float healthBarX = -barWidth / 2 + healthBarWidth / 2;

    glm::fvec4 healthColor;

    if (healthPercentage > 0.5f) {
        healthColor = glm::fvec4(0.6f, 0.2f, 1.0f, 1.0f); // Bright purple
    } else if (healthPercentage > 0.25f) {
        healthColor = glm::fvec4(0.8f, 0.3f, 0.8f, 1.0f); // Pink-purple
    } else {
        healthColor = glm::fvec4(1.0f, 0.2f, 0.6f, 1.0f); // Red-purple (critical)
    }

    if (gameState.bossHealth > 0) {
        drawRectWithGlow(healthBarX, barY, healthBarWidth, barHeight, healthColor, 0.05f, 1.0f);
    }

    glDisable(GL_BLEND);

    glPopMatrix();
}
