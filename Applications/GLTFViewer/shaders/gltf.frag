#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord0;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(baseColorTexture, inTexCoord0);
}
