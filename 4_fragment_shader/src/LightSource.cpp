#include "base.hpp"
#include <string>

LightSource::LightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor, glm::fvec3 specularColor,
                         float intensity) {
    this->ambientColor = ambientColor;
    this->diffuseColor = diffuseColor;
    this->specularColor = specularColor;
    this->intensity = intensity;
}

DirectionalLightSource::DirectionalLightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor,
                                               glm::fvec3 specularColor, float intensity,
                                               glm::fvec3 direction)
    : LightSource(ambientColor, diffuseColor, specularColor, intensity) {

    this->direction = direction;
    this->type = DIRECTIONAL_LIGHT;
}

void DirectionalLightSource::setUniforms(const ShaderProgram &program, int index) const {
    std::string base = "lights[" + std::to_string(index) + "]";
    
    program.setUniform(base + ".type", (int)type);
    program.setUniform(base + ".direction", direction);
    program.setUniform(base + ".ambient", ambientColor);
    program.setUniform(base + ".diffuse", diffuseColor);
    program.setUniform(base + ".specular", specularColor);
    program.setUniform(base + ".intensity", intensity);
    program.setUniform(base + ".enabled", enabled ? 1 : 0);
    
    // Dummy values for unused fields to prevent issues if shader expects them
    program.setUniform(base + ".position", glm::vec3(0.0f));
    program.setUniform(base + ".attenuation", glm::vec3(1.0f, 0.0f, 0.0f)); // Constant attenuation only
}

PointLightSource::PointLightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor,
                                   glm::fvec3 specularColor, float intensity, glm::fvec3 position)
    : LightSource(ambientColor, diffuseColor, specularColor, intensity) {
    this->position = position;
    this->type = POINT_LIGHT;
}

glm::fvec3 PointLightSource::getAttenuation() { 
    // Constant, Linear, Quadratic
    return glm::fvec3(1.0f, 0.09f, 0.032f); 
}

void PointLightSource::setUniforms(const ShaderProgram &program, int index) const {
    std::string base = "lights[" + std::to_string(index) + "]";

    program.setUniform(base + ".type", (int)type);
    program.setUniform(base + ".position", position);
    program.setUniform(base + ".ambient", ambientColor);
    program.setUniform(base + ".diffuse", diffuseColor);
    program.setUniform(base + ".specular", specularColor);
    program.setUniform(base + ".intensity", intensity);
    program.setUniform(base + ".enabled", enabled ? 1 : 0);
    
    // Attenuation
    // glm::vec3 att = const_cast<PointLightSource*>(this)->getAttenuation(); // ugliness
    // Just use hardcoded or member
    program.setUniform(base + ".attenuation", glm::vec3(1.0f, 0.09f, 0.032f)); 

    // Dummy values
    program.setUniform(base + ".direction", glm::vec3(0.0f));
}