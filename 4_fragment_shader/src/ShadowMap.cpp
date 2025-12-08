#include "ShadowMap.hpp"
#include <iostream>

ShadowMap::ShadowMap()
    : depthFBO_(0), depthTexture_(0), resolution_(2048), initialized_(false) {
    prevViewport_[0] = 0;
    prevViewport_[1] = 0;
    prevViewport_[2] = 0;
    prevViewport_[3] = 0;
}

ShadowMap::~ShadowMap() {
    deleteResources();
}

void ShadowMap::init(int resolution) {
    if (initialized_) {
        deleteResources();
    }

    resolution_ = resolution;

    // Create framebuffer
    glGenFramebuffers(1, &depthFBO_);

    // Create depth texture
    createDepthTexture();

    // Attach depth texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture_, 0);

    // No color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow map framebuffer not complete! Status: " << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    initialized_ = true;
    std::cout << "Shadow map initialized (" << resolution << "x" << resolution << ")" << std::endl;
}

void ShadowMap::createDepthTexture() {
    glGenTextures(1, &depthTexture_);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 resolution_, resolution_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Shadow map specific parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to border with white color (no shadow outside map)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware PCF (percentage closer filtering)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void ShadowMap::deleteResources() {
    if (depthTexture_) {
        glDeleteTextures(1, &depthTexture_);
        depthTexture_ = 0;
    }
    if (depthFBO_) {
        glDeleteFramebuffers(1, &depthFBO_);
        depthFBO_ = 0;
    }
    initialized_ = false;
}

void ShadowMap::beginShadowPass() {
    if (!initialized_) return;

    // Save current viewport
    glGetIntegerv(GL_VIEWPORT, prevViewport_);

    // Bind shadow FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO_);
    glViewport(0, 0, resolution_, resolution_);

    // Clear depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // Enable depth test and disable color writing
    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Optional: cull front faces to reduce peter-panning
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void ShadowMap::endShadowPass() {
    if (!initialized_) return;

    // Restore culling (disable it - let other code enable it if needed)
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    // Restore color writing
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Restore viewport
    glViewport(prevViewport_[0], prevViewport_[1], prevViewport_[2], prevViewport_[3]);
}

void ShadowMap::bindTexture(GLenum textureUnit) const {
    if (!initialized_) return;

    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
}

glm::mat4 ShadowMap::calculateLightSpaceMatrix(const glm::vec3& lightDir,
                                                const glm::vec3& sceneCenter,
                                                float orthoSize,
                                                float nearPlane,
                                                float farPlane) const {
    // For directional light, use orthographic projection
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize,
                                            -orthoSize, orthoSize,
                                            nearPlane, farPlane);

    // Position the light "camera" looking at scene center from light direction
    glm::vec3 normalizedDir = glm::normalize(lightDir);
    glm::vec3 lightPos = sceneCenter - normalizedDir * 5.0f; // Back up from scene

    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}

glm::mat4 ShadowMap::calculatePointLightSpaceMatrix(const glm::vec3& lightPos,
                                                     const glm::vec3& targetPos,
                                                     float fov,
                                                     float nearPlane,
                                                     float farPlane) const {
    // For point light, use perspective projection
    float aspect = 1.0f; // Square shadow map
    glm::mat4 lightProjection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);

    // Compute up vector that avoids gimbal lock
    glm::vec3 direction = glm::normalize(targetPos - lightPos);
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
    // If direction is too close to up, use alternative up vector
    if (std::abs(glm::dot(direction, up)) > 0.99f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::mat4 lightView = glm::lookAt(lightPos, targetPos, up);

    return lightProjection * lightView;
}
