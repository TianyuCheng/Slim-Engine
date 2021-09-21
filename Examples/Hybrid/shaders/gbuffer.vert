#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

layout(set = 1, binding = 0) uniform Model {
    mat4 M;
    mat4 N;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outWorldNormal;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec2 outUV;

void main() {
    vec4 worldPos = model.M * vec4(inPosition, 1.0);
    gl_Position = camera.P * camera.V * worldPos;
    outWorldPos = worldPos.xyz;
    outWorldNormal = vec3(model.M * vec4(inNormal, 0.0));
    outUV = inUV;
}
