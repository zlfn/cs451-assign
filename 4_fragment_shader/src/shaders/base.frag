#version 430 core
in vec3 fragColor;
in vec2 fragTexCoord;
out vec4 color;

uniform vec3 objectColor;
uniform float useVertexColor;  // 0.0 = use objectColor/Texture, 1.0 = use input vertex color (fragColor as is)
uniform float useTexture;      // 1.0 = use texture, 0.0 = use objectColor
uniform sampler2D colorSampler;

void main() {
    vec4 baseColor;
    if (useTexture > 0.5) {
        baseColor = texture(colorSampler, fragTexCoord);
        // Combine with objectColor if needed? Usually replacing it.
        // Multiply by objectColor for tinting?
        baseColor.rgb *= objectColor; 
    } else {
        baseColor = vec4(objectColor, 1.0);
    }

    if (useVertexColor > 0.5) {
        // Vertex color mode (e.g. Gouraud shading result passed via fragColor)
        // OR simple vertex colors.
        // For Gouraud: fragColor contains (Ambient+Diffuse+Specular).
        // Result = baseColor * fragColor?
        // Wait, fragColor in Gouraud is light intensity.
        // So:
        color = vec4(baseColor.rgb * fragColor, baseColor.a);
    } else {
        // Flat color mode (e.g. lines)
        color = baseColor;
    }
}