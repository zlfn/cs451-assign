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
