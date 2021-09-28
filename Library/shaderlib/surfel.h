#ifndef SLIM_SHADER_LIB_SURFEL_H
#define SLIM_SHADER_LIB_SURFEL_H

// reference: https://github.com/turanszkij/WickedEngine

#include "glsl.hpp"

// configuration
const uint SURFEL_CAPACITY           = 250000;
const uint SURFEL_CAPACITY_SQRT      = 500;
const uint SURFEL_CELL_CAPACITY      = 0xffffffff;
const uint SURFEL_UPDATE_GROUP_SIZE  = 64;
const float SURFEL_MAX_RADIUS        = 1.0;
const float SURFEL_TARGET_COVERAGE   = 0.5;

// surfel moment
// A texture is used to store moment information for all surfels.
// Each moment information is stored in a 6x6 sub-texture.
const uint SURFEL_MOMENT_TEXEL       = 6;
const uint SURFEL_MOMENT_ATLAS_TEXEL = 6 * SURFEL_CAPACITY_SQRT;

// surfel acceleration structure
const uvec3 SURFEL_GRID_DIMENSIONS   = ivec3(128, 64, 128);
const uint  SURFEL_TABLE_SIZE        = SURFEL_GRID_DIMENSIONS.x * SURFEL_GRID_DIMENSIONS.y * SURFEL_GRID_DIMENSIONS.z;

// surfel tile size
const int SURFEL_TILE_X              = 16;
const int SURFEL_TILE_Y              = 16;

// surfel representation
struct Surfel {
    vec3  position;  // 3-component
    uint  normal;    // 1-component (also served as padding)
    vec3  color;     // 3-component
    float radius;    // 1-component (also served as padding)
};

// surfel data
struct SurfelData {
    // NOTE: we don't support skining now.
    // In the future, we will replace this with object's instance ID,
    // and compute skinning on the fly.
    // For now, we will stick to static scene
    vec3 position;
    uint normal;

    vec3 mean;
    uint life;

    vec3 shortMean;
    float vbbr;

    vec3 variance;
    float inconsistency;

    vec3 hitPos;
    uint hitNormal;

    vec3 hitEnergy;
    float padding0;

    vec3 traceResult;
    float padding1;
};

// surfel grid
struct SurfelGrid {
    uint count;
    uint offset;
};

// surfel stat
struct SurfelStat {
    uint count;
    uint cellAllocator;
};

#ifndef __cplusplus

// checking if a grid is out of range
SLIM_ATTR bool is_surfel_grid_valid(ivec3 grid) {
    if (grid.x < 0 || grid.x >= SURFEL_GRID_DIMENSIONS.x)
        return false;
    if (grid.y < 0 || grid.y >= SURFEL_GRID_DIMENSIONS.y)
        return false;
    if (grid.z < 0 || grid.z >= SURFEL_GRID_DIMENSIONS.z)
        return false;
    return true;
}

// compute the grid index3 for a given world position
SLIM_ATTR ivec3 compute_surfel_grid(vec3 position, vec3 camPos) {
    return ivec3(floor((position - floor(camPos)) / SURFEL_MAX_RADIUS))
         + ivec3(SURFEL_GRID_DIMENSIONS) / 2;
}

// flattens the index3 grid to 1d grid index
SLIM_ATTR uint compute_surfel_grid_index(ivec3 grid) {
    return uint(grid.x) * SURFEL_GRID_DIMENSIONS.y * SURFEL_GRID_DIMENSIONS.z
         + uint(grid.y) * SURFEL_GRID_DIMENSIONS.z
         + uint(grid.z);
}

// check if surfel intersects with grid cell
SLIM_ATTR bool intersect_surfel_grid(Surfel surfel, ivec3 cell, vec3 camPos) {
    vec3 gridmin = cell       - SURFEL_GRID_DIMENSIONS / 2 * SURFEL_MAX_RADIUS + floor(camPos);
    vec3 gridmax = (cell + 1) - SURFEL_GRID_DIMENSIONS / 2 * SURFEL_MAX_RADIUS + floor(camPos);

    vec3 closestPointInAabb = min(max(surfel.position, gridmin), gridmax);
	float dist = distance(closestPointInAabb, surfel.position);
	return dist < surfel.radius;
}

#endif

#endif // SLIM_SHADER_LIB_SURFEL_H
