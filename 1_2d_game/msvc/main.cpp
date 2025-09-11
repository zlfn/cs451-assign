#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

struct Drawable {
    /// Virtual function to draw the object
    virtual void draw() = 0;
    virtual ~Drawable() = default;
};

struct Triangle : public Drawable {
    glm::vec2 v1, v2, v3;
    float depth;
    glm::vec3 color1, color2, color3;

    Triangle(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, float depth = 0.0f,
             const glm::vec3 &color = glm::vec3(0.0f, 1.0f, 1.0f))
        : v1(v1), v2(v2), v3(v3), depth(depth), color1(color), color2(color), color3(color) {}

    Triangle(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, float depth,
             const glm::vec3 &col1, const glm::vec3 &col2, const glm::vec3 &col3)
        : v1(v1), v2(v2), v3(v3), depth(depth), color1(col1), color2(col2), color3(col3) {}

    void draw() override {
        glBegin(GL_TRIANGLES);
        glColor3fv(glm::value_ptr(color1));
        glVertex3f(v1.x, v1.y, depth);
        glColor3fv(glm::value_ptr(color2));
        glVertex3f(v2.x, v2.y, depth);
        glColor3fv(glm::value_ptr(color3));
        glVertex3f(v3.x, v3.y, depth);
        glEnd();
    }
};

struct Square : public Drawable {
    glm::vec2 v1, v2, v3, v4;
    float depth;
    glm::vec3 color1, color2, color3, color4;

    Square(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, const glm::vec2 &v4,
           float depth = 0.0f, const glm::vec3 &color = glm::vec3(0.0f, 1.0f, 1.0f))
        : v1(v1), v2(v2), v3(v3), v4(v4), depth(depth), color1(color), color2(color), color3(color),
          color4(color) {}

    Square(const glm::vec2 &v1, const glm::vec2 &v2, const glm::vec2 &v3, const glm::vec2 &v4,
           float depth, const glm::vec3 &col1, const glm::vec3 &col2, const glm::vec3 &col3,
           const glm::vec3 &col4)
        : v1(v1), v2(v2), v3(v3), v4(v4), depth(depth), color1(col1), color2(col2), color3(col3),
          color4(col4) {}

    void draw() override {
        glBegin(GL_QUADS);
        glColor3fv(glm::value_ptr(color1));
        glVertex3f(v1.x, v1.y, depth);
        glColor3fv(glm::value_ptr(color2));
        glVertex3f(v2.x, v2.y, depth);
        glColor3fv(glm::value_ptr(color3));
        glVertex3f(v3.x, v3.y, depth);
        glColor3fv(glm::value_ptr(color4));
        glVertex3f(v4.x, v4.y, depth);
        glEnd();
    }
};

struct Circle : public Drawable {
    glm::vec2 center;
    float radius;
    float depth;
    glm::vec3 color;
    int segments;

    Circle(const glm::vec2 &center, float radius, float depth = 0.0f,
           const glm::vec3 &color = glm::vec3(0.0f, 1.0f, 1.0f), int segments = 36)
        : center(center), radius(radius), depth(depth), color(color), segments(segments) {}

    void draw() override {
        glBegin(GL_TRIANGLE_FAN);
        glColor3fv(glm::value_ptr(color));
        glVertex3f(center.x, center.y, depth);
        for (int i = 0; i <= segments; ++i) {
            float angle = i * 2.0f * 3.1415926f / segments;
            float x = center.x + radius * cos(angle);
            float y = center.y + radius * sin(angle);
            glVertex3f(x, y, depth);
        }
        glEnd();
    }
};

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::vector<std::unique_ptr<Drawable>> drawables;
    drawables.push_back(
        std::make_unique<Circle>(glm::vec2(0.0f, 0.0f), 0.3f, -0.5f, glm::vec3(1.0f, 1.0f, 0.0f)));

    drawables.push_back(std::make_unique<Triangle>(glm::vec2(-0.5f, -0.5f), glm::vec2(0.5f, -0.5f),
                                                   glm::vec2(0.0f, 0.5f), 0.0f,
                                                   glm::vec3(0.0f, 1.0f, 1.0f)));

    drawables.push_back(std::make_unique<Square>(glm::vec2(-0.5f, -0.5f), glm::vec2(0.5f, -0.5f),
                                                 glm::vec2(0.5f, 0.5f), glm::vec2(-0.5f, 0.5f),
                                                 0.5f, glm::vec3(1.0f, 0.0f, 1.0f)));

    for (const auto &drawable : drawables) {
        drawable->draw();
    }

    glutSwapBuffers();
    glutPostRedisplay();
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(600, 600);
    glutCreateWindow("FreeGLUT + GLEW + GLM Example");

    // Test GLM properly linked
    glm::vec3 glmTest(1.0f, 0.0f, 0.0f);

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
