#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 2, binding = 0) uniform samplerCube skyboxMap;
layout(set = 2, binding = 1) uniform Refraction {
    float iota;
} refraction;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inView;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 v = normalize(inView);
    vec3 n = normalize(inNormal);
    vec3 r = refract(v, n, refraction.iota);
    outColor = texture(skyboxMap, r);
}
