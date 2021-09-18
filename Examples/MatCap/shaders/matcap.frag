#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

layout(set = 2, binding = 0) uniform sampler2D matcap;

layout(location = 0) in  vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(inNormal);
    normal = (camera.V * vec4(normal, 0.0)).xyz;
    vec2 mUV = (normal * 0.5 + 0.5).xy;
    vec4 color = texture(matcap, vec2(mUV.x, 1.0 - mUV.y));
    outColor = color;
}
