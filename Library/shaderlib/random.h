#ifndef SLIM_SHADER_LIB_RANDOM_H
#define SLIM_SHADER_LIB_RANDOM_H

#include "glsl.hpp"

#ifndef __cplusplus

SLIM_ATTR highp float noise(vec2 co)
{
    highp float a = 12.9898;
    highp float b = 78.233;
    highp float c = 43758.5453;
    highp float dt = dot(co.xy ,vec2(a,b));
    highp float sn = mod(dt,3.14);
    return fract(sin(sn) * c);
}

#endif // __cplusplus

#endif // SLIM_SHADER_LIB_RANDOM_H
