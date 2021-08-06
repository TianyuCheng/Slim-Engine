#version 450
#extension GL_ARB_separate_shader_objects : enable

// material set
layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;

// varyings
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV0;

// output
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(baseColorTexture, inUV0);
}
