#ifndef SLIM_SHADER_LIB_UNPACK_H
#define SLIM_SHADER_LIB_UNPACK_H

#include "glsl.hpp"

SLIM_ATTR vec2 unpack_unitvec2(uint value) {
    float r = float((value      ) & 0xff) / 255.0;
    float g = float((value >> 8 ) & 0xff) / 255.0;
    return vec2(r, g);
}

SLIM_ATTR vec3 unpack_unitvec3(uint value) {
    float r = float((value      ) & 0xff) / 255.0;
    float g = float((value >> 8 ) & 0xff) / 255.0;
    float b = float((value >> 16) & 0xff) / 255.0;
    return vec3(r, g, b);
}

SLIM_ATTR vec4 unpack_unitvec4(uint value) {
    float r = float((value      ) & 0xff) / 255.0;
    float g = float((value >> 8 ) & 0xff) / 255.0;
    float b = float((value >> 16) & 0xff) / 255.0;
    float a = float((value >> 24) & 0xff) / 255.0;
    return vec4(r, g, b, a);
}

#endif // SLIM_SHADER_LIB_UNPACK_H
