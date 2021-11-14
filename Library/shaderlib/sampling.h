#ifndef SLIM_SHADER_LIB_SAMPLING_H
#define SLIM_SHADER_LIB_SAMPLING_H

#include "glsl.hpp"

#ifndef __cplusplus

// copied and modified from pbrt
// [u, v] in range [0, 1]
SLIM_ATTR vec3 uniform_sample_hemisphere(float u, float v, out float pdf) {
    float phi = 2.0 * PI * v;
    float r = sqrt(1.0 - u * u);
    pdf = Inv4PI;
    return vec3(r * cos(phi), r * sin(phi), u);
}
SLIM_ATTR vec3 uniform_sample_hemisphere(float u, float v) {
    float pdf = 0.0;
    return uniform_sample_hemisphere(u, v, pdf);
}

// copied and modified from pbrt
// [u, v] in range [0, 1]
SLIM_ATTR vec3 cosine_sample_hemisphere(float u, float v, out float pdf) {
    float theta = v * 2.0 * PI;
    float r = sqrt(u);
    pdf = InvPI * cos(theta);
    return vec3(r * cos(theta), r * sin(theta), 1.0 - u);
}
SLIM_ATTR vec3 cosine_sample_hemisphere(float u, float v) {
    float pdf = 0.0;
    return cosine_sample_hemisphere(u, v, pdf);
}

#endif
#endif // SLIM_SHADER_LIB_SAMPLING_H
