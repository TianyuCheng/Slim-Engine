#ifndef SLIM_SHADER_LIB_PACK_H
#define SLIM_SHADER_LIB_PACK_H

#include "glsl.hpp"

SLIM_ATTR uint pack_unitvec2(vec2 value) {
    uint r = uint(value.r * 255);
    uint g = uint(value.g * 255);
    return r | (g << 8);
}

SLIM_ATTR uint pack_unitvec3(vec3 value) {
    uint r = uint(value.r * 255);
    uint g = uint(value.g * 255);
    uint b = uint(value.b * 255);
    return r | (g << 8) | (b << 16);
}

SLIM_ATTR uint pack_unitvec4(vec4 value) {
    uint r = uint(value.r * 255);
    uint g = uint(value.g * 255);
    uint b = uint(value.b * 255);
    uint a = uint(value.a * 255);
    return r | (g << 8) | (b << 16) | (a << 24);
}

#endif // SLIM_SHADER_LIB_PACK_H
