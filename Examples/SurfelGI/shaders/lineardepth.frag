#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "camera.h"

// camera information
layout(set = 0, binding = 1) uniform Camera {
    mat4 V;
    mat4 P;
    mat4 invVP; // inverse(P * V) for world position reconstruction
    vec3 pos;
    float zNear;
    float zFar;
    float zFarRcp;
} camera;

layout(set = 0, binding = 0) uniform sampler2D imageDepth;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    float depth = texture(imageDepth, inUV).x;
    float linear = linearize_depth(depth, camera.zNear, camera.zFar) * camera.zFarRcp;
    outColor = vec4(linear, linear, linear, 1.0);
}
