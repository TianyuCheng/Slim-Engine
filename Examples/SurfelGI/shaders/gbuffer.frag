#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 2, binding = 0) uniform sampler2D mainTex;

layout(location = 0) in vec3 inWorldNormal;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outWorldNormal;
layout(location = 2) out vec4 outWorldPos;

void main() {
    outAlbedo = texture(mainTex, inUV);
    outWorldNormal = vec4(normalize(inWorldNormal), 1.0);
    outWorldPos = vec4(inWorldPos, 1.0);
}
