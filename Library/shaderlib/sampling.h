#ifndef SLIM_SHADER_LIB_SAMPLING_H
#define SLIM_SHADER_LIB_SAMPLING_H

#include "glsl.hpp"

#ifndef __cplusplus

SLIM_ATTR vec3 sample_hemisphere(float u1, float u2, inout float pdf) {
	float r = sqrt(u1);
	float theta = 2.0 * PI * u2;
	float x = r * cos(theta);
	float y = r * sin(theta);
	vec3 dir = vec3(x, y, sqrt(max(0.0, 1.0 - u1)));
    pdf = dir.z / PI;
    return dir;
}

SLIM_ATTR vec3 sample_hemisphere(float u1, float u2) {
    float pdf;
    return sample_hemisphere(u1, u2, pdf);
}

#endif
#endif // SLIM_SHADER_LIB_SAMPLING_H
