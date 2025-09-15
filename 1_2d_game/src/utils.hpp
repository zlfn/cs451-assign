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

void drawTriangle(glm::fvec2 center, float size, glm::fvec4 color) {
    glColor4f(color.x, color.y, color.z, color.w);
    glBegin(GL_TRIANGLES);
    glVertex2f(center.x, center.y + size / 2);
    glVertex2f(center.x - size / 2, center.y - size / 2);
    glVertex2f(center.x + size / 2, center.y - size / 2);
    glEnd();
}

void drawSpaceship(glm::fvec2 center, float size, glm::fvec4 color) {
    // Enable lighting for 3D effect
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    // Set light properties
    GLfloat lightPos[] = {0.0f, 1.0f, 1.0f, 0.0f};
    GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    // Set material properties with alpha
    GLfloat matAmbient[] = {color.x * 0.3f, color.y * 0.3f, color.z * 0.3f, color.w};
    GLfloat matDiffuse[] = {color.x, color.y, color.z, color.w};
    GLfloat matSpecular[] = {1.0f, 1.0f, 1.0f, color.w};
    GLfloat matShininess[] = {80.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    float s = size * 0.5f;  // Half size
    float d = size * 0.25f; // Depth

    glBegin(GL_TRIANGLES);

    // Main body - front sharp point
    glNormal3f(0.0f, 0.3f, 0.8f);
    glVertex3f(center.x, center.y + s * 1.2f, d * 0.5f); // Sharp nose
    glVertex3f(center.x - s * 0.3f, center.y, d);
    glVertex3f(center.x + s * 0.3f, center.y, d);

    // Main body - top surface
    glNormal3f(0.0f, 0.8f, 0.2f);
    glVertex3f(center.x, center.y + s * 1.2f, d * 0.5f); // Sharp nose
    glVertex3f(center.x - s * 0.3f, center.y, d);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);

    glVertex3f(center.x, center.y + s * 1.2f, d * 0.5f); // Sharp nose
    glVertex3f(center.x + s * 0.3f, center.y, d);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);

    // Left wing
    glNormal3f(-0.7f, 0.3f, 0.2f);
    glVertex3f(center.x - s * 0.3f, center.y, d);
    glVertex3f(center.x - s * 1.0f, center.y - s * 0.8f, d * 0.3f);
    glVertex3f(center.x - s * 0.5f, center.y - s * 0.8f, -d);

    glVertex3f(center.x - s * 0.3f, center.y, d);
    glVertex3f(center.x - s * 0.5f, center.y - s * 0.8f, -d);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);

    // Right wing
    glNormal3f(0.7f, 0.3f, 0.2f);
    glVertex3f(center.x + s * 0.3f, center.y, d);
    glVertex3f(center.x + s * 1.0f, center.y - s * 0.8f, d * 0.3f);
    glVertex3f(center.x + s * 0.5f, center.y - s * 0.8f, -d);

    glVertex3f(center.x + s * 0.3f, center.y, d);
    glVertex3f(center.x + s * 0.5f, center.y - s * 0.8f, -d);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);

    // Bottom surface
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(center.x - s * 1.0f, center.y - s * 0.8f, d * 0.3f);
    glVertex3f(center.x + s * 1.0f, center.y - s * 0.8f, d * 0.3f);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);

    glVertex3f(center.x - s * 1.0f, center.y - s * 0.8f, d * 0.3f);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);
    glVertex3f(center.x - s * 0.5f, center.y - s * 0.8f, -d);

    glVertex3f(center.x + s * 1.0f, center.y - s * 0.8f, d * 0.3f);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);
    glVertex3f(center.x + s * 0.5f, center.y - s * 0.8f, -d);

    // Vertical stabilizer (tail fin)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(center.x, center.y + s * 0.2f, -d * 0.5f);
    glVertex3f(center.x, center.y - s * 0.5f, -d * 1.2f);
    glVertex3f(center.x, center.y - s * 0.3f, -d * 0.8f);

    glEnd();

    // Cockpit window
    glDisable(GL_LIGHTING);
    glBegin(GL_TRIANGLES);
    glColor4f(0.2f, 0.4f, 0.8f, color.w);
    glVertex3f(center.x, center.y + s * 0.9f, d * 0.6f);
    glVertex3f(center.x - s * 0.15f, center.y + s * 0.3f, d * 0.8f);
    glVertex3f(center.x + s * 0.15f, center.y + s * 0.3f, d * 0.8f);
    glEnd();

    // Engine exhausts
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Left engine
    glBegin(GL_TRIANGLES);
    glColor4f(0.2f, 0.6f, 1.0f, 0.9f * color.w);
    glVertex3f(center.x - s * 0.3f, center.y - s * 0.6f, -d * 0.8f);
    glVertex3f(center.x - s * 0.15f, center.y - s * 0.6f, -d * 0.8f);
    glColor4f(0.0f, 0.2f, 0.8f, 0.1f * color.w);
    glVertex3f(center.x - s * 0.225f, center.y - s * 1.3f, -d * 1.2f);
    glEnd();

    // Right engine
    glBegin(GL_TRIANGLES);
    glColor4f(0.2f, 0.6f, 1.0f, 0.9f * color.w);
    glVertex3f(center.x + s * 0.3f, center.y - s * 0.6f, -d * 0.8f);
    glVertex3f(center.x + s * 0.15f, center.y - s * 0.6f, -d * 0.8f);
    glColor4f(0.0f, 0.2f, 0.8f, 0.1f * color.w);
    glVertex3f(center.x + s * 0.225f, center.y - s * 1.3f, -d * 1.2f);
    glEnd();

    // Center engine
    glBegin(GL_TRIANGLES);
    glColor4f(0.4f, 0.7f, 1.0f, 1.0f * color.w);
    glVertex3f(center.x - s * 0.08f, center.y - s * 0.4f, -d * 0.9f);
    glVertex3f(center.x + s * 0.08f, center.y - s * 0.4f, -d * 0.9f);
    glColor4f(0.0f, 0.3f, 0.9f, 0.1f * color.w);
    glVertex3f(center.x, center.y - s * 1.2f, -d * 1.3f);
    glEnd();

    glDisable(GL_BLEND);
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
