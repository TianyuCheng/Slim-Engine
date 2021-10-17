#ifndef SLIM_SHADER_LIB_TRANSFORM_H
#define SLIM_SHADER_LIB_TRANSFORM_H

#include "glsl.hpp"

SLIM_ATTR vec4 compute_rotation_to_z_axis(vec3 v) {
    // handle special case when input is exactly (0, 0, 1)
    if (v.z < -0.99999) return vec4(1.0, 0.0, 0.0, 0.0);
    // rest of the cases
    return normalize(vec4(v.y, -v.x, 0.0, 1.0 + v.z));
}

SLIM_ATTR vec4 invert_rotation(vec4 q) {
    return vec4(-q.x, -q.y, -q.z, q.w);
}

SLIM_ATTR vec3 rotate_point(vec4 q, vec3 v) {
    const vec3 qAxis = vec3(q.x, q.y, q.z);
    return qAxis * 2.0f * dot(qAxis, v) + (q.w * q.w - dot(qAxis, qAxis)) * v + 2.0f * q.w * cross(qAxis, v);
}

#endif // SLIM_SHADER_LIB_TRANSFORM_H
