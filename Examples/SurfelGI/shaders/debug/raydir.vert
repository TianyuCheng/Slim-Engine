#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

#include "../common.h"

// camera information
layout(set = 0, binding = SCENE_CAMERA_BINDING) uniform
Camera { CameraInfo data; } camera;

layout(location = 0) in vec3 inVertex;

void main() {
    gl_Position = camera.data.P * camera.data.V * vec4(inVertex, 1.0);
}
