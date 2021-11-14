#version 460
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../common.h"

layout(push_constant) uniform Object { uint instanceID; } object;

// Camera
layout(set = 0, binding = SCENE_CAMERA_BINDING) uniform
Camera { CameraInfo data; } camera;

// Instances
layout(set = 0, binding = SCENE_INSTANCE_BINDING) buffer
Instance { InstanceInfo data[]; } instances;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;
#ifdef ENABLE_GBUFFER_WORLD_POSITION
layout(location = 2) out vec3 outPosition;
#endif

void main() {
    InstanceInfo instance = instances.data[object.instanceID];

    mat4 P = camera.data.P;
    mat4 V = camera.data.V;
    mat4 M = instance.M;
    mat4 N = instance.N;
    gl_Position = P * V * M * vec4(inPosition, 1.0);

    // varyings
    outNormal = vec3(N * vec4(inNormal, 0.0));
    outUV     = inUV;
    #ifdef ENABLE_GBUFFER_WORLD_POSITION
    outPosition = vec3(M * vec4(inPosition, 1.0));
    #endif
}
