#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform samplerCube skyboxMap;

layout(location = 0) in vec3 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(skyboxMap, inTexCoord);
}
