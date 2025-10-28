#include "base.hpp"
#include "utils.hpp"

ThreeDObj::ThreeDObj(const std::string filePath)
    : objectColor(1.0f, 1.0f, 1.0f) // 흰색
{
    try {
        getObjFile(filePath);
    } catch (const std::exception &e) {
        std::cerr << "Error loading object: " << e.what() << std::endl;
    }
}

void ThreeDObj::getObjFile(const std::string filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    baseVertices.clear();
    indices.clear();
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            baseVertices.push_back(vertex);

        } else if (prefix == "f") {
            std::vector<unsigned int> faceIndices;
            std::string vertexToken;
            while (ss >> vertexToken) {
                std::stringstream tokenSS(vertexToken);
                std::string indexStr;
                std::getline(tokenSS, indexStr, '/');
                faceIndices.push_back(std::stoul(indexStr) - 1);
            }

            for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                indices.push_back(faceIndices[0]);
                indices.push_back(faceIndices[i]);
                indices.push_back(faceIndices[i + 1]);
            }
        }
    }
    file.close();
    std::cout << "Loaded " << baseVertices.size() << " vertices, " << (indices.size() / 3)
              << " triangles from " << filePath << std::endl;
}

void ThreeDObj::setColor(const glm::vec3 &color) { objectColor = color; }

void ThreeDObj::draw() {
    if (baseVertices.empty() || indices.empty()) {
        return;
    }

    glColor3f(objectColor.x, objectColor.y, objectColor.z);

    // 삼각형 와이어프레임 그리기
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    {
        for (unsigned int index : indices) {
            const glm::vec3 &vertex = baseVertices[index];
            glVertex3f(vertex.x, vertex.y, vertex.z);
        }
    }
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

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

// a, b를 포함하는 범위에서 랜덤 정수 반환
int getRandomRange(int a, int b) {
    static std::uniform_int_distribution<int> distInt(a, b);
    return static_cast<int>(dist(gen));
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
