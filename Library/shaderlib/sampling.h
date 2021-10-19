#ifndef SLIM_SHADER_LIB_SAMPLING_H
#define SLIM_SHADER_LIB_SAMPLING_H

#include "glsl.hpp"

#ifndef __cplusplus

// [u, v] in range [0, 1]
SLIM_ATTR vec3 uniform_sample_hemisphere(float u, float v) {
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * cosTheta, cosTheta);
}

// [u, v] in range [0, 1]
SLIM_ATTR vec3 cosine_sample_hemisphere(float u, float v) {
    float phi = v * 2.0 * PI;
    float cosTheta = sqrt(1.0 - u);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * cosTheta, cosTheta);
}


#endif
#endif // SLIM_SHADER_LIB_SAMPLING_H
