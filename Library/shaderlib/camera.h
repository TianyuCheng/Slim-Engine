#ifndef SLIM_SHADER_LIB_CAMERA_H
#define SLIM_SHADER_LIB_CAMERA_H

#include "glsl.hpp"

SLIM_ATTR float linearize_depth(float depth, float z_near, float z_far)
{
#if 0
    float z_n = 2.0 * depth - 1.0;
    float linear = 2.0 * z_far * z_near / (z_near + z_far - z_n * (z_near - z_far));
    return linear;
#else
    return z_near * z_far / (z_far + depth * (z_near - z_far));
#endif
}

SLIM_ATTR vec3 reconstruct_position(vec2 uv, float depth, mat4 invVP) {
    float x = uv.x * 2.0 - 1.0;
    float y = (1.0 - uv.y) * 2.0 - 1.0;
    float z = depth;
    vec4 position_s = vec4(x, y, z, 1.0);
    vec4 position_v = invVP * position_s;
    return vec3(position_v) / position_v.w;
}

#endif // SLIM_SHADER_LIB_CAMERA_H
