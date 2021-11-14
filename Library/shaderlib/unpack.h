#ifndef SLIM_SHADER_LIB_UNPACK_H
#define SLIM_SHADER_LIB_UNPACK_H

#include "glsl.hpp"

SLIM_ATTR vec2 unpack_unorm2(uint value) {
    return unpackUnorm2x16(value);
}

SLIM_ATTR vec3 unpack_unorm3(uint value) {
    return vec3(unpackUnorm4x8(value));
}

SLIM_ATTR vec4 unpack_unorm4(uint value) {
    return unpackUnorm4x8(value);
}

SLIM_ATTR vec2 unpack_snorm2(uint value) {
    return unpackSnorm2x16(value);
}

SLIM_ATTR vec3 unpack_snorm3(uint value) {
    return vec3(unpackSnorm4x8(value));
}

SLIM_ATTR vec4 unpack_snorm4(uint value) {
    return unpackSnorm4x8(value);
}

#endif // SLIM_SHADER_LIB_UNPACK_H
