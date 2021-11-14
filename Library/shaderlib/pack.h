#ifndef SLIM_SHADER_LIB_PACK_H
#define SLIM_SHADER_LIB_PACK_H

#include "glsl.hpp"

// value: [0, 1]^2
SLIM_ATTR uint pack_unorm2(vec2 value) {
    return packUnorm2x16(value);
}

// value: [0, 1]^3
SLIM_ATTR uint pack_unorm3(vec3 value) {
    return packUnorm4x8(vec4(value, 0.0));
}

// value: [0, 1]^4
SLIM_ATTR uint pack_unorm4(vec4 value) {
    return packUnorm4x8(value);
}

// value: [0, 1]^2
SLIM_ATTR uint pack_snorm2(vec2 value) {
    return packSnorm2x16(value);
}

// value: [0, 1]^3
SLIM_ATTR uint pack_snorm3(vec3 value) {
    return packSnorm4x8(vec4(value, 0.0));
}

// value: [0, 1]^4
SLIM_ATTR uint pack_snorm4(vec4 value) {
    return packSnorm4x8(value);
}

#endif // SLIM_SHADER_LIB_PACK_H
