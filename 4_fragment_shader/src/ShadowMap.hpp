#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "graphics.hpp"

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    // Initialize shadow map with given resolution
    void init(int resolution = 2048);

    // Begin shadow pass - bind FBO and set viewport
    void beginShadowPass();

    // End shadow pass - unbind FBO and restore viewport
    void endShadowPass();

    // Bind shadow map texture for sampling in shading pass
    void bindTexture(GLenum textureUnit = GL_TEXTURE1) const;

    // Calculate light space matrix for directional light
    glm::mat4 calculateLightSpaceMatrix(const glm::vec3& lightDir,
                                         const glm::vec3& sceneCenter,
                                         float orthoSize = 3.0f,
                                         float nearPlane = 0.1f,
                                         float farPlane = 10.0f) const;

    // Calculate light space matrix for point light (perspective projection)
    glm::mat4 calculatePointLightSpaceMatrix(const glm::vec3& lightPos,
                                              const glm::vec3& targetPos,
                                              float fov = 90.0f,
                                              float nearPlane = 0.01f,
                                              float farPlane = 10.0f) const;

    GLuint getTextureID() const { return depthTexture_; }
    int getResolution() const { return resolution_; }
    bool isInitialized() const { return initialized_; }

private:
    void createDepthTexture();
    void deleteResources();

    GLuint depthFBO_;
    GLuint depthTexture_;
    int resolution_;
    bool initialized_;

    // Store previous viewport to restore after shadow pass
    GLint prevViewport_[4];
};
