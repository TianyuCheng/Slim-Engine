#ifndef SLIM_SHADER_LIB_SAMPLING_H
#define SLIM_SHADER_LIB_SAMPLING_H

#include "glsl.hpp"

SLIM_ATTR vec3 uniform_sample_hemisphere(float u1, float u2) {
    float r = sqrt(1.0 - u1 * u1);
    float phi = 2 * PI * u2;
    vec3 dir = vec3(cos(phi) * r, sin(phi) * r, u1);
    return dir;
}

SLIM_ATTR vec3 cosine_sample_hemisphere(float u1, float u2) {
	float r = sqrt(u1);
	float theta = 2 * PI * u2;
	float x = r * cos(theta);
	float y = r * sin(theta);
	vec3 dir = vec3(x, y, sqrt(max(0.0, 1.0 - u1)));
    return dir;
}

#endif // SLIM_SHADER_LIB_SAMPLING_H
