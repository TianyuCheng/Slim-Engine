#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "surfel.h"

// surfel stat
layout(set = 0, binding = 0) buffer SurfelStatBuffer {
    SurfelStat data;
} surfelStat;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    float ratio = float(surfelStat.data.count) / float(SURFEL_CAPACITY);
    if (inUV.x > ratio) {
        discard;
        return;
    }
    float r = mix(0.0, 1.0, ratio);
    float g = mix(1.0, 0.0, ratio);
    outColor = vec4(r, g, 0.0, 1.0);
}
