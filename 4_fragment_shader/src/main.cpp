#include "utils.hpp"
#include "base.hpp"
#include "graphics.hpp"
#include "PostProcess.hpp"
#include "ShadowMap.hpp"
#include "stb_image.h"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

GameState::GameState(int h, int bh)
    : MAX_PLAYER_HEALTH(h), MAX_BOSS_HEALTH(bh), playerHealth(h), bossHealth(bh),
      cameraBaseOffset(0.0f, 0.0f), cameraShakeOffset(0.0f, 0.0f),
      playerObject(glm::fvec2(0.0f, -0.8f)), bossObject1(glm::fvec2(0.5f, 0.6f), 1),
      bossObject2(glm::fvec2(-0.5f, 0.6f), 2), bossHealthBarObject(glm::fvec2(0.0f, 0.0f)),
      heartsObject(glm::fvec2(0.0f, 0.0f)) {}

MatrixStack modelViewStack;
MatrixStack projectionStack;

enum ProjMethod { DIAG_PERSPECTIVE, TOP_PERSPECTIVE, TOP_PARALLEL };
ProjMethod currentProjMethod = DIAG_PERSPECTIVE;

enum ShadingStyle { GOURAUD, PHONG, PHONG_WITH_NORMAL };
ShadingStyle currentShadingStyle = GOURAUD;

int keyPressDelay = 0;
int renderModeKeyDelay = 0;

float playerSpeedBase = 0.00065f;
bool isCameraShake = false;
int cameraShakeStartTime = 0;
bool keyStates[256] = {false};
void showVictoryScreen(const GameState &gameState);

// Global Shader Programs
std::unique_ptr<ShaderProgram> programGouraud;
std::unique_ptr<ShaderProgram> programPhong;
std::unique_ptr<ShaderProgram> programPhongN;
std::unique_ptr<ShaderProgram> programSimple;
std::unique_ptr<ShaderProgram> programMotionBlur;

// Motion blur processor
MotionBlurProcessor motionBlurProcessor;
float motionBlurStrength = 0.3f; // Adjustable blur strength (0.0 - 0.5)
bool motionBlurEnabled = true;
int motionBlurKeyDelay = 0;

// Shadow mapping
std::unique_ptr<ShaderProgram> programShadowDepth;
ShadowMap shadowMap;
bool shadowEnabled = true;
int shadowKeyDelay = 0;
glm::mat4 g_lightSpaceMatrix = glm::mat4(1.0f);

// Ground plane for shadow receiving
std::unique_ptr<Mesh> groundPlaneMesh;
GLuint groundDiffuseTexture = 0;
GLuint groundNormalTexture = 0;

// Global Shader Program Pointer (used by objects)
ShaderProgram* g_shaderProgram = nullptr;

void startCameraShake(int currentTime) {
    isCameraShake = true;
    cameraShakeStartTime = currentTime;
}

glm::fvec2 cameraShake(int currentTime) {
    int deltaTime = currentTime - cameraShakeStartTime;
    if (deltaTime > 2000) {
        isCameraShake = false;
    }
    float offset =
        0.5f / (static_cast<float>(deltaTime) / 2.0f - 20.0f * std::numbers::pi_v<float>)*std::sin(
                   static_cast<float>(deltaTime) / 2.0f - 20.0f * std::numbers::pi_v<float>);
    return glm::fvec2(offset, 0.0);
}

GameState gameState(5, 200);

CommandExecutor commandExecutor({'w', 'w', 's', 's', 'a', 'd', 'a', 'd', 'b', 'a'},
                                [](GameState &gameState) {
                                    if (gameState.playerObject.isDying)
                                        return;

                                    gameState.MAX_PLAYER_HEALTH = 10;
                                    gameState.playerHealth = 10;
                                    gameState.konamiUsed = true;

                                    int currentTime = glutGet(GLUT_ELAPSED_TIME);
                                    gameState.playerObject.isInvincible = true;
                                    gameState.playerObject.invincibilityEndTime =
                                        currentTime + 5000;
                                    gameState.playerObject.bulletCount = 5;

                                    std::cout << "↑↑↓↓←→←→BA" << '\n';
                                });

void keyboardDown(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = true; }
void keyboardUp(unsigned char key, int /*x*/, int /*y*/) { keyStates[key] = false; }

float smoothProjRotChange() {
    int dt = glutGet(GLUT_ELAPSED_TIME) - keyPressDelay;
    if (dt > 500 || currentProjMethod == TOP_PARALLEL) {
        return 0.0f;
    } else {
        return 50.0 * (1.0 - ((float)dt / 500.0));
    }
}

float smoothProjScaleChange() {
    int dt = glutGet(GLUT_ELAPSED_TIME) - keyPressDelay;
    if (dt > 500 || currentProjMethod == TOP_PARALLEL) {
        return 2.0f;
    } else {
        return 2.0 + (1.0 - ((float)dt / 500.0));
    }
}

float smoothProjZDistChange() {
    int dt = glutGet(GLUT_ELAPSED_TIME) - keyPressDelay;
    if (dt > 500 || currentProjMethod == TOP_PARALLEL) {
        return -1.0f;
    } else {
        return -0.5f - 0.5f * ((float)dt / 500.0);
    }
}

GLuint loadTexture(const std::string& path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void initGroundPlane() {
    // Load industrial textures for ground plane
    groundDiffuseTexture = loadTexture("assets/diffuse_secondary.png");
    groundNormalTexture = loadTexture("assets/normal_industrial.png");

    // Ground plane vertices: position (3) + normal (3) + texcoord (2)
    float vertices[] = {
        // Triangle 1
        -2.0f, -2.0f, -0.1f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         2.0f, -2.0f, -0.1f,   0.0f, 0.0f, 1.0f,   4.0f, 0.0f,
         2.0f,  2.0f, -0.1f,   0.0f, 0.0f, 1.0f,   4.0f, 4.0f,
        // Triangle 2
        -2.0f, -2.0f, -0.1f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         2.0f,  2.0f, -0.1f,   0.0f, 0.0f, 1.0f,   4.0f, 4.0f,
        -2.0f,  2.0f, -0.1f,   0.0f, 0.0f, 1.0f,   0.0f, 4.0f,
    };

    groundPlaneMesh = std::make_unique<Mesh>();
    groundPlaneMesh->setData(vertices, sizeof(vertices), GL_STATIC_DRAW);
    groundPlaneMesh->setAttribute(0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
    groundPlaneMesh->setAttribute(1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    groundPlaneMesh->setAttribute(3, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    groundPlaneMesh->setDrawMode(GL_TRIANGLES, 6);
}

void drawGroundPlane() {
    if (!groundPlaneMesh || !g_shaderProgram) return;

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();
    glm::mat4 normalMat = modelViewStack.getTopNormal();

    // Bind ground textures explicitly
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, groundDiffuseTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, groundNormalTexture);

    drawMesh(*groundPlaneMesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("normalMatrix", normalMat);
        prog.setUniform("objectColor", glm::vec3(0.4f, 0.4f, 0.45f));
        prog.setUniform("useTexture", 1.0f);
        prog.setUniform("colorSampler", 0);
        prog.setUniform("normalSampler", 1);
    });
}

void setupLights(ShaderProgram& program, const glm::mat4& viewMatrix) {
    program.use();
    // Assuming numLights is used, but we loop MAX_LIGHTS in shader with 'enabled' check
    // program.setUniform("numLights", (int)gameState.lights.size());

    for (size_t i = 0; i < gameState.lights.size(); ++i) {
        gameState.lights[i]->setUniforms(program, (int)i);
    }

    // Environment reflection setup
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gameState.skyboxObject.getTextureID());
    program.setUniform("environmentMap", 2);
    program.setUniform("reflectivity", 0.3f);

    // Pass inverse view matrix for world-space reflection
    glm::mat4 inverseView = glm::inverse(viewMatrix);
    program.setUniform("inverseViewMatrix", inverseView);

    // Shadow mapping setup
    if (shadowEnabled && shadowMap.isInitialized()) {
        shadowMap.bindTexture(GL_TEXTURE3);
        program.setUniform("shadowMap", 3);
        program.setUniform("shadowEnabled", 1);
        // Combine lightSpaceMatrix with inverseView to transform from view space to light space
        glm::mat4 lightSpaceViewMatrix = g_lightSpaceMatrix * inverseView;
        program.setUniform("lightSpaceViewMatrix", lightSpaceViewMatrix);
    } else {
        program.setUniform("shadowEnabled", 0);
    }
}

void display() {
    const float SCALE = currentProjMethod == DIAG_PERSPECTIVE
        ? 3.0 : smoothProjScaleChange();
    const float CAMERA_ANGLE_X_DEG = currentProjMethod == DIAG_PERSPECTIVE
        ? 50.0f : smoothProjRotChange();
    const float Z_DIST_VIEW = currentProjMethod == DIAG_PERSPECTIVE
        ? -0.5f : smoothProjZDistChange();

    const float ANGLE_RAD = (float)(CAMERA_ANGLE_X_DEG * (std::numbers::pi / 180.0f));
    const float Y_COMPENSATION = (std::tan(ANGLE_RAD) * std::abs(Z_DIST_VIEW)) / SCALE;
    const float Z_TRANSLATE = Z_DIST_VIEW / std::cos(ANGLE_RAD) / SCALE;

    // ============ Shadow Pass ============
    if (shadowEnabled && shadowMap.isInitialized() && gameState.lights.size() > 1) {
        // Use point light (orbiting light) for shadow calculation
        auto* pointLight = dynamic_cast<PointLightSource*>(gameState.lights[1].get());
        if (pointLight) {
            // Calculate light space matrix for point light looking at ground plane
            glm::vec3 lightPos = pointLight->position;

            // Ground plane is at z = -0.1, spans from -2 to 2
            // We need to see all corners from the light position
            float groundZ = -0.1f;
            float groundSize = 2.0f; // half-size of ground

            // Calculate the most distant corner from light position on ground plane
            glm::vec2 lightXY(lightPos.x, lightPos.y);
            float maxCornerDist = 0.0f;
            for (int i = -1; i <= 1; i += 2) {
                for (int j = -1; j <= 1; j += 2) {
                    glm::vec2 corner(groundSize * i, groundSize * j);
                    float dist = glm::length(corner - lightXY);
                    maxCornerDist = std::max(maxCornerDist, dist);
                }
            }

            // Height from light to ground
            float heightToGround = lightPos.z - groundZ;

            // FOV needed to see the farthest corner
            float halfFovRad = std::atan(maxCornerDist / heightToGround);
            float fov = 2.0f * glm::degrees(halfFovRad) * 1.1f; // 10% margin
            fov = glm::clamp(fov, 60.0f, 175.0f);

            // Target point is on the ground directly below the light
            glm::vec3 targetPos(lightPos.x * 0.3f, lightPos.y * 0.3f, groundZ);

            g_lightSpaceMatrix = shadowMap.calculatePointLightSpaceMatrix(
                lightPos, targetPos, fov, 0.05f, 20.0f);

            shadowMap.beginShadowPass();

            // Use shadow depth shader
            g_shaderProgram = programShadowDepth.get();
            programShadowDepth->use();

            // Set up projection as lightSpaceMatrix, objects will use modelView for their transforms
            projectionStack.loadIdentity();
            projectionStack.matMul(g_lightSpaceMatrix);
            modelViewStack.loadIdentity();

            // Draw all shadow-casting objects
            for (auto &object : gameState.enemyBulletObjects) {
                object.draw(gameState);
            }
            for (auto &object : gameState.playerBulletObjects) {
                object.draw(gameState);
            }
            gameState.bossObject1.draw(gameState);
            gameState.bossObject2.draw(gameState);
            gameState.playerObject.draw(gameState);

            shadowMap.endShadowPass();
        }
    }

    // ============ Main Render Pass ============
    // Begin rendering to FBO if motion blur is enabled
    if (motionBlurEnabled && motionBlurProcessor.isInitialized()) {
        motionBlurProcessor.beginScene();
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3D Projection
    projectionStack.loadIdentity();
    glm::mat4 projection;
    if (currentProjMethod == TOP_PARALLEL) {
        projection = glm::ortho(-SCALE, SCALE, -SCALE, SCALE, -5.0f, 5.0f);
    } else {
        projection = glm::frustum(-0.2f, 0.2f, -0.2f, 0.2f, 0.1f, 20.0f);
    }
    projectionStack.matMul(projection);

    modelViewStack.loadIdentity();

    // Skybox
    auto drawSkybox = [&]() {
        modelViewStack.matPush();
        modelViewStack.rotate(-CAMERA_ANGLE_X_DEG, 1.0f, 0.0f, 0.0f);
        gameState.skyboxObject.draw(gameState);
        modelViewStack.matPop();
    };

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawSkybox();

    // 3D Objects View Transform
    modelViewStack.matPush();
    glm::fvec2 &cbo = gameState.cameraBaseOffset;
    modelViewStack.rotate(-CAMERA_ANGLE_X_DEG, 1.0f, 0.0f, 0.0f); 
    modelViewStack.scale(SCALE, SCALE, SCALE);
    modelViewStack.translate(-cbo.x, -cbo.y + Y_COMPENSATION, Z_TRANSLATE);
    modelViewStack.translate(-gameState.cameraShakeOffset.x, -gameState.cameraShakeOffset.y, 0.0f);

    // 1. Draw Unlit Objects (Background, Particles)
    g_shaderProgram = programSimple.get();
    gameState.backgroundObject.draw(gameState);

    // Trail particles (Uncomment if you want them)
    // for (auto &particle : gameState.trailParticles) { particle.draw(gameState); }

    // 2. Draw Lit Objects (Player, Boss, Bullets)
    glDisable(GL_BLEND);

    switch (currentShadingStyle) {
    case GOURAUD:
        g_shaderProgram = programGouraud.get();
        break;
    case PHONG:
        g_shaderProgram = programPhong.get();
        break;
    case PHONG_WITH_NORMAL:
        g_shaderProgram = programPhongN.get();
        break;
    }
    glm::mat4 currentViewMatrix = modelViewStack.getTopMatrix();
    setupLights(*g_shaderProgram, currentViewMatrix);

    // Draw ground plane first (receives shadows)
    drawGroundPlane();

    for (auto &object : gameState.enemyBulletObjects) {
        object.draw(gameState);
    }
    for (auto &object : gameState.playerBulletObjects) {
        object.draw(gameState);
    }
    gameState.bossObject1.draw(gameState);
    gameState.bossObject2.draw(gameState);
    gameState.playerObject.draw(gameState);

    modelViewStack.matPop(); 

    // 2D Projection (UI)
    projectionStack.loadIdentity();
    projectionStack.matMul(glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    modelViewStack.loadIdentity();
    
    // UI uses simple shader
    g_shaderProgram = programSimple.get();
    
    gameState.bossHealthBarObject.draw(gameState);
    gameState.heartsObject.draw(gameState);

    // Apply motion blur post-processing
    if (motionBlurEnabled && motionBlurProcessor.isInitialized()) {
        motionBlurProcessor.endSceneAndApply(*programMotionBlur, motionBlurStrength);
    }

    glutSwapBuffers();
    if (!motionBlurEnabled) {
        glutPostRedisplay();
    }
}

void keyInputUpdate(int dt) {
    float playerSpeed = playerSpeedBase * static_cast<float>(dt);
    if (keyStates[27]) {
        std::cout << "ESC pressed -> exit\n";
        std::exit(0);
    }
    if (keyStates['w']) {
        gameState.playerObject.move(glm::vec2(0.0f, playerSpeed));
    }

    bool movingHorizontal = false;

    if (keyStates['a']) {
        gameState.playerObject.move(glm::vec2(-playerSpeed, 0.0f));
        gameState.playerObject.targetTiltAngle = 25.0f;
        movingHorizontal = true;
    }
    if (keyStates['s']) {
        gameState.playerObject.move(glm::vec2(0.0f, -playerSpeed));
    }
    if (keyStates['d']) {
        gameState.playerObject.move(glm::vec2(playerSpeed, 0.0f));
        gameState.playerObject.targetTiltAngle = -25.0f;
        movingHorizontal = true;
    }

    int now = glutGet(GLUT_ELAPSED_TIME);
    if (keyStates['c'] && keyPressDelay + 500 < now) {
        keyPressDelay = now;
        std::cout << "Change projection method to ";
        switch (currentProjMethod) {
        case DIAG_PERSPECTIVE:
            currentProjMethod = TOP_PERSPECTIVE;
            std::cout << "'top perspective'" << std::endl;
            break;
        case TOP_PERSPECTIVE:
            currentProjMethod = TOP_PARALLEL;
            std::cout << "'top parallel'" << std::endl;
            break;
        case TOP_PARALLEL:
            currentProjMethod = DIAG_PERSPECTIVE;
            std::cout << "'diagonal perspective'" << std::endl;
            break;
        }
    }

    if (keyStates['e'] && renderModeKeyDelay + 500 < now) {
        renderModeKeyDelay = now;
        std::cout << "Change shading style to ";
        switch (currentShadingStyle) {
        case GOURAUD:
            currentShadingStyle = PHONG;
            std::cout << "'phong without normal map'" << std::endl;
            break;
        case PHONG:
            currentShadingStyle = PHONG_WITH_NORMAL;
            std::cout << "'phong with normal map'" << std::endl;
            break;
        case PHONG_WITH_NORMAL:
            currentShadingStyle = GOURAUD;
            std::cout << "'gouraud'" << std::endl;
            break;
        }
    }

    if (!movingHorizontal) {
        gameState.playerObject.targetTiltAngle = 0.0f;
    }

    if (keyStates[' ']) {
        gameState.playerObject.tryAttack();
    }

    // Toggle motion blur with 'm' key
    if (keyStates['m'] && motionBlurKeyDelay + 500 < now) {
        motionBlurKeyDelay = now;
        motionBlurEnabled = !motionBlurEnabled;
        std::cout << "Motion blur " << (motionBlurEnabled ? "enabled" : "disabled") << std::endl;
    }

    // Adjust motion blur strength with '[' and ']' keys
    if (keyStates['['] && motionBlurKeyDelay + 100 < now) {
        motionBlurKeyDelay = now;
        motionBlurStrength = std::max(0.0f, motionBlurStrength - 0.05f);
        std::cout << "Motion blur strength: " << motionBlurStrength << std::endl;
    }
    if (keyStates[']'] && motionBlurKeyDelay + 100 < now) {
        motionBlurKeyDelay = now;
        motionBlurStrength = std::min(0.7f, motionBlurStrength + 0.05f);
        std::cout << "Motion blur strength: " << motionBlurStrength << std::endl;
    }

    // Toggle shadows with 'n' key
    if (keyStates['n'] && shadowKeyDelay + 500 < now) {
        shadowKeyDelay = now;
        shadowEnabled = !shadowEnabled;
        std::cout << "Shadows " << (shadowEnabled ? "enabled" : "disabled") << std::endl;
    }
}

void updateOrbitingLight(int currentTime) {
    // Point light (index 1) orbits around the player
    if (gameState.lights.size() > 1) {
        auto* pointLight = dynamic_cast<PointLightSource*>(gameState.lights[1].get());
        if (pointLight) {
            float orbitRadius = 1.0f;
            float orbitSpeed = 0.002f; // radians per ms
            float angle = currentTime * orbitSpeed;

            glm::fvec2 playerPos = gameState.playerObject.currentPosition;
            float x = playerPos.x + orbitRadius * std::cos(angle);
            float y = playerPos.y + orbitRadius * std::sin(angle);
            float z = 1.5f; // height above the plane (higher for better shadow coverage)

            pointLight->position = glm::vec3(x, y, z);
        }
    }
}

void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    static int lastMs = now;

    auto bossMoveData1 = getCurrentMove(now, gameState, 1);
    auto bossMoveData2 = getCurrentMove(now, gameState, 2);
    if (bossMoveData1.has_value()) {
        gameState.bossObject1.currentMove = bossMoveData1.value();
    }
    if (bossMoveData2.has_value()) {
        gameState.bossObject2.currentMove = bossMoveData2.value();
    }
    if (isCameraShake) {
        gameState.cameraShakeOffset = cameraShake(now);
    }

    // Update orbiting light position
    updateOrbitingLight(now);

    int dt = now - lastMs;
    lastMs = now;

    keyInputUpdate(dt);

    std::erase_if(gameState.enemyBulletObjects,
                  [&](auto &it) { return it.update(now, gameState); });

    std::erase_if(gameState.playerBulletObjects,
                  [&](auto &it) { return it.update(now, gameState); });

    std::erase_if(gameState.trailParticles, [&](auto &it) { return it.update(now, gameState); });

    gameState.backgroundObject.update(now, gameState);
    gameState.playerObject.update(now, gameState);
    gameState.bossObject1.update(now, gameState);
    gameState.bossObject2.update(now, gameState);

    commandExecutor.update(now, gameState);

    glutPostRedisplay();
    glutTimerFunc(motionBlurEnabled ? 30 : 16, timer, 0);
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    if (motionBlurProcessor.isInitialized()) {
        motionBlurProcessor.resize(width, height);
    }
}

void initShaders() {
    try {
        std::cout << "Loading Simple Shader...\n";
        programSimple = std::make_unique<ShaderProgram>();
        programSimple->attachShader(Shader::fromSource(Shader::Type::VERTEX, shaders::SIMPLE_VERT_SHADER));
        programSimple->attachShader(Shader::fromSource(Shader::Type::FRAGMENT, shaders::BASE_FRAG_SHADER));
        programSimple->link();

        std::cout << "Loading Gouraud Shader...\n";
        programGouraud = std::make_unique<ShaderProgram>();
        programGouraud->attachShader(Shader::fromSource(Shader::Type::VERTEX, shaders::GOURAUD_VERT_SHADER));
        programGouraud->attachShader(Shader::fromSource(Shader::Type::FRAGMENT, shaders::BASE_FRAG_SHADER));
        programGouraud->link();

        std::cout << "Loading Phong Shader...\n";
        programPhong = std::make_unique<ShaderProgram>();
        programPhong->attachShader(Shader::fromSource(Shader::Type::VERTEX, shaders::PHONG_VERT_SHADER));
        programPhong->attachShader(Shader::fromSource(Shader::Type::FRAGMENT, shaders::PHONG_FRAG_SHADER));
        programPhong->link();

        std::cout << "Loading PhongN Shader...\n";
        programPhongN = std::make_unique<ShaderProgram>();
        programPhongN->attachShader(Shader::fromSource(Shader::Type::VERTEX, shaders::PHONG_VERT_SHADER));
        programPhongN->attachShader(Shader::fromSource(Shader::Type::FRAGMENT, shaders::PHONGN_FRAG_SHADER));
        programPhongN->link();

        std::cout << "Loading Motion Blur Shader...\n";
        programMotionBlur = std::make_unique<ShaderProgram>();
        programMotionBlur->attachShader(Shader::fromSource(Shader::Type::VERTEX, shaders::MOTIONBLUR_VERT_SHADER));
        programMotionBlur->attachShader(Shader::fromSource(Shader::Type::FRAGMENT, shaders::MOTIONBLUR_FRAG_SHADER));
        programMotionBlur->link();

        std::cout << "Loading Shadow Depth Shader...\n";
        programShadowDepth = std::make_unique<ShaderProgram>();
        programShadowDepth->attachShader(Shader::fromSource(Shader::Type::VERTEX, shaders::SHADOW_DEPTH_VERT_SHADER));
        programShadowDepth->attachShader(Shader::fromSource(Shader::Type::FRAGMENT, shaders::SHADOW_DEPTH_FRAG_SHADER));
        programShadowDepth->link();

        std::cout << "Shaders initialized successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "Shader initialization failed: " << e.what() << '\n';
        // Pause to let user see the error if running from IDE
        std::cout << "Press Enter to exit...";
        std::cin.get();
        std::exit(1);
    }
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1400, 1400);
    glutCreateWindow("CSED451 Assn 4");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패: " << glewGetErrorString(err) << '\n';
        return -1;
    }

    // 셰이더 초기화
    initShaders();

    // Lights
    // Directional (Sun-like)
    gameState.lights.push_back(std::make_shared<DirectionalLightSource>(
        glm::vec3(0.15f, 0.15f, 0.15f), // Ambient
        glm::vec3(0.7f, 0.7f, 0.7f),    // Diffuse
        glm::vec3(0.8f, 0.8f, 0.8f),    // Specular
        1.0f,                           // Intensity
        glm::vec3(-0.5f, -1.0f, -0.5f)  // Direction
    ));

    // Point Light (e.g. glowing projectile or center light)
    gameState.lights.push_back(std::make_shared<PointLightSource>(
        glm::vec3(0.0f, 0.0f, 0.0f),    // Ambient
        glm::vec3(0.8f, 0.8f, 0.8f),    // Diffuse (White)
        glm::vec3(0.7f, 0.7f, 0.7f),    // Specular
        1.5f,                           // Intensity
        glm::vec3(0.0f, 0.0f, 0.5f)     // Position (Above player start)
    ));

    glEnable(GL_DEPTH_TEST);

    // Initialize motion blur processor
    motionBlurProcessor.init(1400, 1400);
    motionBlurKeyDelay = glutGet(GLUT_ELAPSED_TIME);

    // Initialize shadow map
    shadowMap.init(2048);
    shadowKeyDelay = glutGet(GLUT_ELAPSED_TIME);

    // Initialize ground plane with industrial texture
    initGroundPlane();

    keyPressDelay = glutGet(GLUT_ELAPSED_TIME);
    renderModeKeyDelay = glutGet(GLUT_ELAPSED_TIME);

    // Load skybox
    if (!gameState.skyboxObject.load("assets/skybox")) {
        std::cerr << "Failed to load skybox textures\n";
    }

    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
