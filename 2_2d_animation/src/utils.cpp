#include "base.hpp"
#include "utils.hpp"

void showVictoryScreen(const GameState &gameState) {
    int elapsedTime = glutGet(GLUT_ELAPSED_TIME);
    int seconds = elapsedTime / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    std::cout << "\n\n";
    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[1;33m"
              << "                                                                             \n";
    std::cout << "\033[1;33m"
              << "       ██╗   ██╗██╗ ██████╗████████╗ ██████╗ ██████╗ ██╗   ██╗██╗            \n";
    std::cout << "\033[1;33m"
              << "       ██║   ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██║            \n";
    std::cout << "\033[1;33m"
              << "       ██║   ██║██║██║        ██║   ██║   ██║██████╔╝ ╚████╔╝ ██║            \n";
    std::cout << "\033[1;33m"
              << "       ╚██╗ ██╔╝██║██║        ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚═╝            \n";
    std::cout << "\033[1;33m"
              << "        ╚████╔╝ ██║╚██████╗   ██║   ╚██████╔╝██║  ██║   ██║   ██╗            \n";
    std::cout << "\033[1;33m"
              << "         ╚═══╝  ╚═╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝            \n";
    std::cout << "\033[1;33m"
              << "                                                                             \n";
    if (gameState.konamiUsed) {
        std::cout
            << "\033[1;35m"
            << "                             ↑↑↓↓←→←→BA                                    \n";
    } else {
        std::cout << "\033[1;35m"
                  << "                          ⟡ BOSS DEFEATED ⟡                              \n";
    }
    std::cout << "\033[1;36m"
              << "============================================================================\n\n";

    std::cout << "\033[1;32m" << "                        ╔════════════════════╗\n";
    std::cout << "\033[1;32m" << "                        ║   GAME STATISTICS  ║\n";
    std::cout << "\033[1;32m" << "                        ╚════════════════════╝\n\n";

    std::cout << "\033[1;37m" << "                    ⏱  Clear Time: " << "\033[1;33m";
    std::cout << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0')
              << std::setw(2) << seconds << "\033[0m\n\n";

    std::cout << "\033[1;37m" << "                    ❤  Lives Remaining: " << "\033[1;31m";
    for (int i = 0; i < gameState.playerHealth; i++) {
        std::cout << "♥";
    }
    std::cout << " (" << gameState.playerHealth << "/" << gameState.MAX_PLAYER_HEALTH
              << ")\033[0m\n\n";

    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[1;35m"
              << "                      Thank you for playing!                              \n";
    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[0m\n\n";
}

int getRandomRange(int a, int b) { // a, b 포함
    static std::uniform_int_distribution<int> dist_int(a, b);
    return dist(gen);
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
    float halfW = width / 2.0f;
    float halfH = height / 2.0f;

    // Draw outer glow quad (fully transparent at edges)
    glBegin(GL_QUADS);

    // Top edge glow
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x - halfW - glowSize, y + halfH + glowSize, zDepth - 0.01f);
    glVertex3f(x + halfW + glowSize, y + halfH + glowSize, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x + halfW, y + halfH, zDepth - 0.01f);
    glVertex3f(x - halfW, y + halfH, zDepth - 0.01f);

    // Bottom edge glow
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x - halfW, y - halfH, zDepth - 0.01f);
    glVertex3f(x + halfW, y - halfH, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x + halfW + glowSize, y - halfH - glowSize, zDepth - 0.01f);
    glVertex3f(x - halfW - glowSize, y - halfH - glowSize, zDepth - 0.01f);

    // Left edge glow
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x - halfW - glowSize, y - halfH - glowSize, zDepth - 0.01f);
    glVertex3f(x - halfW - glowSize, y + halfH + glowSize, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x - halfW, y + halfH, zDepth - 0.01f);
    glVertex3f(x - halfW, y - halfH, zDepth - 0.01f);

    // Right edge glow
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x + halfW, y - halfH, zDepth - 0.01f);
    glVertex3f(x + halfW, y + halfH, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x + halfW + glowSize, y + halfH + glowSize, zDepth - 0.01f);
    glVertex3f(x + halfW + glowSize, y - halfH - glowSize, zDepth - 0.01f);

    glEnd();

    // Draw corner glows using triangle fans (smoother corners)
    // Top-left corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x - halfW, y + halfH, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x - halfW - glowSize, y + halfH, zDepth - 0.01f);
    glVertex3f(x - halfW - glowSize * 0.7f, y + halfH + glowSize * 0.7f, zDepth - 0.01f);
    glVertex3f(x - halfW, y + halfH + glowSize, zDepth - 0.01f);
    glEnd();

    // Top-right corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x + halfW, y + halfH, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x + halfW, y + halfH + glowSize, zDepth - 0.01f);
    glVertex3f(x + halfW + glowSize * 0.7f, y + halfH + glowSize * 0.7f, zDepth - 0.01f);
    glVertex3f(x + halfW + glowSize, y + halfH, zDepth - 0.01f);
    glEnd();

    // Bottom-left corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x - halfW, y - halfH, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x - halfW, y - halfH - glowSize, zDepth - 0.01f);
    glVertex3f(x - halfW - glowSize * 0.7f, y - halfH - glowSize * 0.7f, zDepth - 0.01f);
    glVertex3f(x - halfW - glowSize, y - halfH, zDepth - 0.01f);
    glEnd();

    // Bottom-right corner
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(color.r, color.g, color.b, color.a * 0.6f);
    glVertex3f(x + halfW, y - halfH, zDepth - 0.01f);
    glColor4f(color.r, color.g, color.b, 0.0f);
    glVertex3f(x + halfW + glowSize, y - halfH, zDepth - 0.01f);
    glVertex3f(x + halfW + glowSize * 0.7f, y - halfH - glowSize * 0.7f, zDepth - 0.01f);
    glVertex3f(x + halfW, y - halfH - glowSize, zDepth - 0.01f);
    glEnd();

    // Draw main rectangle
    glBegin(GL_QUADS);
    glColor4f(color.r, color.g, color.b, color.a);
    glVertex3f(x - halfW, y - halfH, zDepth);
    glVertex3f(x + halfW, y - halfH, zDepth);
    glVertex3f(x + halfW, y + halfH, zDepth);
    glVertex3f(x - halfW, y + halfH, zDepth);
    glEnd();
}
