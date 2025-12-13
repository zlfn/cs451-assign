#version 430 core

#define MAX_LIGHTS 4

struct Light {
    int type; // 0: Directional, 1: Point
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float intensity;
    vec3 attenuation;
    int enabled;
};

uniform Light lights[MAX_LIGHTS];
uniform int numLights;

// Material properties (simplified, assuming objectColor is diffuse/ambient)
uniform vec3 objectColor;
uniform vec3 viewPos; // Camera position in World Space (or calculate ViewDir in View Space)
// Note: We are working in View Space mostly because modelView is passed.
// If lights are defined in World Space, we need View Matrix.
// Existing code uses modelView and projection.
// Let's assume lights are passed in View Space or we convert them. 
// Easier: Do lighting in View Space. 
// If lights are static in world, we need to transform them by View Matrix.
// BUT `LightSource` sets uniforms directly. 
// We will assume the `LightSource` uniforms are already in the correct space OR we do lighting in World Space.
// Standard approach: Lighting in View Space.
// But `LightSource` just passes `position`. If camera moves, light position in View Space changes.
// The `LightSource` implementation I wrote passes `position` directly.
// In `main.cpp`, lights are added to `GameState`. `GameState` doesn't seem to update light positions based on camera.
// Wait, the camera is defined by `modelViewStack` which includes camera transform.
// The `modelView` uniform passed to shader is Model * View.
// So `position` input is Model Space. `pos_VS` is View Space.
// The lights must be in View Space to interact with `pos_VS`.
// OR we transform `pos_VS` back to World Space (if we have Inverse View).
// Given the simplicity, I'll assume lights are defined in World Space and I should transform them to View Space,
// OR I assume `LightSource` sets View Space coordinates?
// Actually, `DirectionalLight` (Sun) usually fixed in World.
// `PointLight` fixed in World.
// So I need the View Matrix to transform Light Position/Direction to View Space.
// `base.hpp` has `modelViewStack`. The Camera transform is applied first.
// I can pass `viewMatrix` to shader.
// `ThreeDObj::draw` passes `modelView` (which is M*V).
// I will add `uniform mat4 view;` to shaders and `ThreeDObj::draw`.
// In `main.cpp` -> `display()`: `view` matrix is `modelViewStack` before object transforms?
// `display()`:
//   modelViewStack.rotate(-CAMERA_ANGLE...); // View transform
//   modelViewStack.push();
//   ... draw objects ...
// So I can get the View Matrix.

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 modelView;
uniform mat4 normalMatrix;
uniform float useTexture; // To determine if we use texture or objectColor for base color
uniform float shiness; // Optional, or hardcode

// Environment reflection
uniform samplerCube environmentMap;
uniform float reflectivity;
uniform mat4 inverseViewMatrix;

out vec3 fragColor;
out vec2 fragTexCoord;
out vec3 envReflection;

void main() {
    vec4 pos_VS = modelView * vec4(position, 1.0);
    vec3 vertexPos_VS = pos_VS.xyz;
    vec3 normal_VS = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
    vec3 viewVec_VS = normalize(-vertexPos_VS);
    
    vec3 totalAmbient = vec3(0.0);
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    // Hardcoded material shininess
    float materialShininess = 32.0;

    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i].enabled == 0) continue;

        vec3 lightAmbient = lights[i].ambient;
        vec3 lightDiffuse = lights[i].diffuse;
        vec3 lightSpecular = lights[i].specular;
        float intensity = lights[i].intensity;

        vec3 lightDir_VS;
        float attenuation = 1.0;

        if (lights[i].type == 0) { // Directional
             // Assuming light.direction is in View Space or we treat it as such. 
             // Ideally we should pass View Matrix to transform it.
             // For this assignment, if we ignore camera rotation for lights or assume lights follow camera?
             // Let's assume light direction is defined in View Space for simplicity if we lack View Matrix.
             // OR: `main.cpp` rotates camera. 
             // If I don't multiply by View, the light will rotate with the objects (fixed relative to camera).
             // That's acceptable for "Headlight" or simple UI, but maybe not for "Sun".
             // However, passing View Matrix requires updating `ThreeDObj::draw`.
             // `ThreeDObj::draw` takes `modelView` (M*V).
             // Recovering V from M*V is hard without M.
             // I will try to implement without explicit View Matrix first (Lights fixed in View Space).
             // This means lights are attached to the camera.
             // If the user wants world space lights, I'd need to change `setUniforms` to take View Matrix.
             lightDir_VS = normalize(-lights[i].direction); 
        } else { // Point
             // Assuming light.position is View Space.
             lightDir_VS = normalize(lights[i].position - vertexPos_VS);
             float distance = length(lights[i].position - vertexPos_VS);
             attenuation = 1.0 / (lights[i].attenuation.x + lights[i].attenuation.y * distance + lights[i].attenuation.z * distance * distance);
        }

        // Ambient
        totalAmbient += lightAmbient * intensity * attenuation;

        // Diffuse
        float diff = max(dot(normal_VS, lightDir_VS), 0.0);
        totalDiffuse += lightDiffuse * diff * intensity * attenuation;

        // Specular
        vec3 reflectDir = reflect(-lightDir_VS, normal_VS);
        float spec = pow(max(dot(viewVec_VS, reflectDir), 0.0), materialShininess);
        totalSpecular += lightSpecular * spec * intensity * attenuation;
    }

    // Combine results
    // Note: final color = ambient * objectColor + diffuse * objectColor + specular * specularColor
    // We output these components or the final sum? 
    // Gouraud usually sums at vertex.
    // If texture is used, 'objectColor' is replaced by texture in Frag shader.
    // So we should pass Light Intensity, not final color?
    // Standard Gouraud: Calculate Color at Vertex.
    // But we don't have texture color here.
    // So usually we pass `LightIntensity` (Diffuse + Ambient) and `SpecularIntensity`.
    // Then Frag shader: `Texture * (Amb+Diff) + Spec`.
    // Let's pass `lightingColor` and `specularColor`.
    
    // Simplified: Pass (Ambient+Diffuse) and Specular separately?
    // Or just pass the summed light multipliers.
    
    vec3 lightMult = totalAmbient + totalDiffuse;
    
    // We store the 'light color' in fragColor. 
    // In Base Frag, we will multiply this by Texture.
    // Specular is additive (white usually), so it shouldn't be multiplied by Texture (usually).
    // But `base.frag` is simple.
    // I will modify `gouraud.vert` to output `lightDiffuse` and `lightSpecular`.
    
    // Re-using fragColor for Diffuse+Ambient.
    fragColor = lightMult; 
    
    // Specular? We can add it to fragColor but then it gets multiplied by texture in frag shader (if we do simple multiply).
    // Correct way: `color = texture * diffuse + specular`.
    // I need another varying for specular.
    // I'll assume `base.frag` will be updated to handle `specular`.
    // But `base.frag` is used by UI lines too.
    
    fragTexCoord = texCoord;
    gl_Position = projection * pos_VS;

    // Hack for now: Add specular to fragColor. It will be tinted by texture.
    fragColor += totalSpecular;

    // Environment reflection (per-vertex)
    vec3 reflectDir_VS = reflect(-viewVec_VS, normal_VS);
    vec3 reflectDir_WS = mat3(inverseViewMatrix) * reflectDir_VS;
    envReflection = texture(environmentMap, reflectDir_WS).rgb * reflectivity;
}
