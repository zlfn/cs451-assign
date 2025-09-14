#include <GL/glew.h>
#include <glm/glm.hpp>
#include <numbers>

void drawCircle(glm::fvec2 center, float radius, int numSegments, glm::fvec3 color) {
    glColor3f(color.x, color.y, color.z);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(center.x, center.y);
    for (int i = 0; i <= numSegments; i++) {
        float angle = static_cast<float>(2.0f * std::numbers::pi * i / numSegments);
        float x = center.x + radius * std::cos(angle);
        float y = center.y + radius * std::sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void drawRect(glm::fvec2 center, float size, glm::fvec3 color) {
    float half = size / 2.0f;

    glColor3f(color.x, color.y, color.z);
    glBegin(GL_TRIANGLES);
    glVertex2f(center.x - half, center.y + half);
    glVertex2f(center.x - half, center.y - half);
    glVertex2f(center.x + half, center.y - half);
    glVertex2f(center.x - half, center.y + half);
    glVertex2f(center.x + half, center.y - half);
    glVertex2f(center.x + half, center.y + half);
    glEnd();
}

void drawTriangle(glm::fvec2 center, float size, glm::fvec3 color) {
    glColor3f(color.x, color.y, color.z);
    glBegin(GL_TRIANGLES);
    glVertex2f(center.x, center.y + size / 2);
    glVertex2f(center.x - size / 2, center.y - size / 2);
    glVertex2f(center.x + size / 2, center.y - size / 2);
    glEnd();
}

void drawRectWithGlow(float x, float y, float width, float height, glm::fvec4 color, float glowSize,
                      float zDepth) {
    // Draw glow effect using gradients
    // Top glow gradient
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++) {
        float t = static_cast<float>(i) / 8.0f;
        float alpha = (1.0f - t) * color.a * 0.6f;
        float offset = t * glowSize;
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(x - width / 2, y + height / 2 + offset, zDepth - 0.01f);
        glVertex3f(x + width / 2, y + height / 2 + offset, zDepth - 0.01f);
    }
    glEnd();

    // Bottom glow gradient
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++) {
        float t = static_cast<float>(i) / 8.0f;
        float alpha = (1.0f - t) * color.a * 0.6f;
        float offset = t * glowSize;
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(x - width / 2, y - height / 2 - offset, zDepth - 0.01f);
        glVertex3f(x + width / 2, y - height / 2 - offset, zDepth - 0.01f);
    }
    glEnd();

    // Left glow gradient
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++) {
        float t = static_cast<float>(i) / 8.0f;
        float alpha = (1.0f - t) * color.a * 0.6f;
        float offset = t * glowSize;
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(x - width / 2 - offset, y - height / 2, zDepth - 0.01f);
        glVertex3f(x - width / 2 - offset, y + height / 2, zDepth - 0.01f);
    }
    glEnd();

    // Right glow gradient
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++) {
        float t = static_cast<float>(i) / 8.0f;
        float alpha = (1.0f - t) * color.a * 0.6f;
        float offset = t * glowSize;
        glColor4f(color.r, color.g, color.b, alpha);
        glVertex3f(x + width / 2 + offset, y - height / 2, zDepth - 0.01f);
        glVertex3f(x + width / 2 + offset, y + height / 2, zDepth - 0.01f);
    }
    glEnd();

    // Corner glows - draw as simple quarter circles
    // Top-Left corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.5f);
    glVertex3f(x - width / 2, y + height / 2, zDepth - 0.01f);

    glColor4f(color.r, color.g, color.b, 0.0f);
    for (int i = 0; i <= 16; i++) {
        float angle = (static_cast<float>(i) / 16.0f) * 3.14159f / 2.0f;
        float cx = x - width / 2.0f - glowSize * std::sinf(angle);
        float cy = y + height / 2.0f + glowSize * std::cosf(angle);
        glVertex3f(cx, cy, zDepth - 0.01f);
    }
    glEnd();

    // Top-Right corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.5f);
    glVertex3f(x + width / 2, y + height / 2, zDepth - 0.01f);

    glColor4f(color.r, color.g, color.b, 0.0f);
    for (int i = 0; i <= 16; i++) {
        float angle = (static_cast<float>(i) / 16.0f) * 3.14159f / 2.0f;
        float cx = x + width / 2.0f + glowSize * std::cosf(angle);
        float cy = y + height / 2.0f + glowSize * std::sinf(angle);
        glVertex3f(cx, cy, zDepth - 0.01f);
    }
    glEnd();

    // Bottom-Left corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.5f);
    glVertex3f(x - width / 2, y - height / 2, zDepth - 0.01f);

    glColor4f(color.r, color.g, color.b, 0.0f);
    for (int i = 0; i <= 16; i++) {
        float angle = (static_cast<float>(i) / 16.0f) * 3.14159f / 2.0f;
        float cx = x - width / 2.0f - glowSize * std::cosf(angle);
        float cy = y - height / 2.0f - glowSize * std::sinf(angle);
        glVertex3f(cx, cy, zDepth - 0.01f);
    }
    glEnd();

    // Bottom-Right corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.5f);
    glVertex3f(x + width / 2, y - height / 2, zDepth - 0.01f);

    glColor4f(color.r, color.g, color.b, 0.0f);
    for (int i = 0; i <= 16; i++) {
        float angle = (static_cast<float>(i) / 16.0f) * 3.14159f / 2.0f;
        float cx = x + width / 2.0f + glowSize * std::sinf(angle);
        float cy = y - height / 2.0f - glowSize * std::cosf(angle);
        glVertex3f(cx, cy, zDepth - 0.01f);
    }
    glEnd();

    // Draw main rectangle
    glBegin(GL_QUADS);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(x - width / 2, y - height / 2, zDepth);
    glVertex3f(x + width / 2, y - height / 2, zDepth);
    glVertex3f(x + width / 2, y + height / 2, zDepth);
    glVertex3f(x - width / 2, y + height / 2, zDepth);
    glEnd();
}

void drawTrapezoidWithGlow(float x, float y, float topWidth, float bottomWidth, float height,
                           glm::fvec4 color, float glowSize, float zDepth) {
    // Draw glow effect
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++) {
        float t = static_cast<float>(i) / 8.0f;
        float alpha = (1.0f - t) * color.a * 0.5f;
        float offset = t * glowSize;

        glColor4f(color.r, color.g, color.b, alpha);
        // Top edge glow
        glVertex3f(x - topWidth / 2 - offset, y + height / 2 + offset, zDepth - 0.01f);
        glVertex3f(x + topWidth / 2 + offset, y + height / 2 + offset, zDepth - 0.01f);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++) {
        float t = static_cast<float>(i) / 8.0f;
        float alpha = (1.0f - t) * color.a * 0.5f;
        float offset = t * glowSize;

        glColor4f(color.r, color.g, color.b, alpha);
        // Bottom edge glow
        glVertex3f(x - bottomWidth / 2 - offset, y - height / 2 - offset, zDepth - 0.01f);
        glVertex3f(x + bottomWidth / 2 + offset, y - height / 2 - offset, zDepth - 0.01f);
    }
    glEnd();

    // Draw main trapezoid
    glBegin(GL_QUADS);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(x - bottomWidth / 2, y - height / 2, zDepth);
    glVertex3f(x + bottomWidth / 2, y - height / 2, zDepth);
    glVertex3f(x + topWidth / 2, y + height / 2, zDepth);
    glVertex3f(x - topWidth / 2, y + height / 2, zDepth);
    glEnd();

    // Draw trapezoid outline
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glVertex3f(x - bottomWidth / 2, y - height / 2, zDepth + 0.001f);
    glVertex3f(x + bottomWidth / 2, y - height / 2, zDepth + 0.001f);
    glVertex3f(x + topWidth / 2, y + height / 2, zDepth + 0.001f);
    glVertex3f(x - topWidth / 2, y + height / 2, zDepth + 0.001f);
    glEnd();
}
