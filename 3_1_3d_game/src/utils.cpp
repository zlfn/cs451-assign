#include "base.hpp"
#include "utils.hpp"

ThreeDObj::ThreeDObj(const std::string &FILE_PATH, const glm::fvec3 &color)
    : objectColor(color) // 기본은 흰색
{
    try {
        getObjFile(FILE_PATH);
    } catch (const std::exception &e) {
        std::cerr << "Error loading object: " << e.what() << '\n';
    }
}

void ThreeDObj::getObjFile(const std::string &FILE_PATH) {
    std::ifstream file(FILE_PATH);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + FILE_PATH);
    }

    std::string currentObjName = "base";

    baseVertices.clear();
    objIndicesMap.clear();
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "o") {
            Indices newIndices;
            ss >> currentObjName;
            objIndicesMap.insert({currentObjName, newIndices});
        } else if (prefix == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            baseVertices.push_back(vertex);
        } else if (prefix == "f" && currentObjName != "") {
            std::vector<unsigned int> faceIndices;
            std::string vertexToken;
            while (ss >> vertexToken) {
                std::stringstream tokenSS(vertexToken);
                std::string indexStr;
                std::getline(tokenSS, indexStr, '/');
                faceIndices.push_back(std::stoul(indexStr) - 1);
            }

            for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                objIndicesMap[currentObjName].push_back(faceIndices[0]);
                objIndicesMap[currentObjName].push_back(faceIndices[i]);
                objIndicesMap[currentObjName].push_back(faceIndices[i + 1]);
            }
        }
    }
    file.close();

    for (const auto &[key, value] : objIndicesMap) {
        Indices currentIndices = value;
        glm::vec3 centerPos = glm::vec3(0.0,0.0,0.0);
        for (const auto &vIndex : currentIndices) {
            centerPos += baseVertices[vIndex];
        }
        centerPos /= currentIndices.size();
        objCenterMap.insert({key, centerPos});
    }

    std::cout << "Loaded " << baseVertices.size() << " vertices, " << (objIndicesMap.size())
              << " objects from " << FILE_PATH << std::endl;
}

void ThreeDObj::setColor(const glm::vec3 &color) { objectColor = color; }

void ThreeDObj::draw(const std::string objName) {
    if (baseVertices.empty() || objIndicesMap.empty()) {
        return;
    }

    Indices indices;
    try {
        indices = objIndicesMap[objName];
    } catch (const std::out_of_range &oor) {
        std::cerr << "Error: there is no object named " << objName << std::endl;
    }

    glLineWidth(1.0f);
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
