#ifndef SLIM_SHADER_LIB_COLORS_H
#define SLIM_SHADER_LIB_COLORS_H

#include "glsl.hpp"

#ifndef __cplusplus

const int random_color_count = 12;

// for random colors
const vec3 random_colors[] = vec3[random_color_count](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(0.2, 0.5, 0.7),
    vec3(0.5, 0.3, 0.1),
    vec3(0.6, 0.4, 0.2),
    vec3(0.4, 0.1, 0.9),
    vec3(0.5, 0.3, 0.7),
    vec3(0.1, 0.7, 0.2)
);

#endif

#endif // SLIM_SHADER_LIB_COLORS_H
