#include <glm/glm.hpp>
#include <vector>

#define M_PI 3.14159265358979323846

void drawCircle(glm::fvec2 center, float radius, int numSegments, glm::fvec3 color) {
    glColor3f(color.x, color.y, color.z);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(center.x, center.y);
    for (int i = 0; i <= numSegments; i++) {
        float angle = 2.0f * M_PI * i / numSegments;
        float x = center.x + radius * cos(angle);
        float y = center.y + radius * sin(angle);
        glVertex2f(x, y);
    }
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
