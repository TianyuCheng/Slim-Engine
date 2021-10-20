#version 450
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "../common.h"

layout(push_constant) uniform Light { uint lightID; } light;

// Lights
layout(set = 0, binding = SCENE_LIGHT_BINDING) buffer
Light { LightInfo data[]; } lights;

layout(location = 0) out vec4 outColor;

void main() {
    LightInfo info = lights.data[light.lightID];
    outColor = vec4(info.color, 1.0);
}
