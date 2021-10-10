#ifndef SLIM_SHADER_LIB_AABB_H
#define SLIM_SHADER_LIB_AABB_H

#include "glsl.hpp"

struct AABB {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

#endif // SLIM_SHADER_LIB_AABB_H
