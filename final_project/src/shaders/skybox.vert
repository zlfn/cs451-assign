#version 450 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    // Remove translation from view matrix
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
    // Set z to w so that after perspective division, z will be 1.0 (maximum depth)
    gl_Position = pos.xyww;
}
