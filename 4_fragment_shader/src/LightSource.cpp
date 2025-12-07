#include "base.hpp"

void addLightSource(LightSource lightSource) {
    
}

LightSource::LightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor, glm::fvec3 specularColor,
                         float intensity) {
    this->ambientColor = ambientColor;
    this->diffuseColor = diffuseColor;
    this->specularColor = specularColor;
    this->intensity = intensity;
}

void LightSource::setUniforms(Shader &shader) {}

DirectionalLightSource::DirectionalLightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor,
                                               glm::fvec3 specularColor, float intensity,
                                               glm::fvec3 direction)
    : LightSource(ambientColor, diffuseColor, specularColor, intensity) {

    this->direction = direction;
    this->type = DIRECTIONAL_LIGHT;
}

PointLightSource::PointLightSource(glm::fvec3 ambientColor, glm::fvec3 diffuseColor,
                                   glm::fvec3 specularColor, float intensity, glm::fvec3 position)
    : LightSource(ambientColor, diffuseColor, specularColor, intensity) {
    this->position = position;
    this->type = POINT_LIGHT;
}

glm::fvec3 PointLightSource::getAttenuation() { return glm::fvec3(1.0); }
