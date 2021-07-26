#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants { mat4 M; } model;

layout(set = 0, binding = 0) uniform Camera {
    mat4 V;
    mat4 P;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord0;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord0;

void main() {
    gl_Position = camera.P * camera.V * model.M * vec4(inPosition, 1.0);
    outNormal = transpose(mat3(camera.V * model.M)) * inNormal;
    outTexCoord0 = vec2(inTexCoord0.x, 1.0 - inTexCoord0.y);
}
