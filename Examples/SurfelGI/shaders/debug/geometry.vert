#version 460
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable

#include "../common.h"

layout(push_constant) uniform Light { uint lightID; } light;

// Camera
layout(set = 0, binding = SCENE_CAMERA_BINDING) uniform
Camera { CameraInfo data; } camera;

// LightXform
layout(set = 0, binding = SCENE_LIGHT_XFORM_BINDING) buffer
LightXform { mat4 data[]; } lightXform;

layout(location = 0) in vec3 inPosition;

void main() {
    mat4 P = camera.data.P;
    mat4 V = camera.data.V;
    mat4 M = lightXform.data[light.lightID];
    gl_Position = P * V * M * vec4(inPosition, 1.0);
}
