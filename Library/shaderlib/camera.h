#ifndef SLIM_SHADER_LIB_CAMERA_H
#define SLIM_SHADER_LIB_CAMERA_H

#include "glsl.hpp"

SLIM_ATTR float linearize_depth(float depth, float z_near, float z_far)
{
#if 0
    // OpenGL style [-1, 1] depth range
    float z_n = 2.0 * depth - 1.0;
    float linear = 2.0 * z_far * z_near / (z_near + z_far - z_n * (z_near - z_far));
    return linear;
#else
    // Vulkan style [0, 1] depth range
    return z_near * z_far / (z_far + depth * (z_near - z_far));
#endif
}

SLIM_ATTR vec3 compute_world_position(vec2 ndc, float depth, mat4 invVP) {
    float x = +(ndc.x * 2.0 - 1.0);
    float y = -(ndc.y * 2.0 - 1.0);
    float z = depth;
    vec4 position_c = vec4(x, y, z, 1.0);
    vec4 position_w = invVP * position_c;
    return vec3(position_w) / position_w.w;
}

#endif // SLIM_SHADER_LIB_CAMERA_H
