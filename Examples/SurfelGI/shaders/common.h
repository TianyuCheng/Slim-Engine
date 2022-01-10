#ifndef SLIM_EXAMPLE_SHADERS_COMMON_H
#define SLIM_EXAMPLE_SHADERS_COMMON_H

#include "../config.h"

#ifdef __cplusplus
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
using uint=uint32_t;
using vec2=glm::vec2;
using vec3=glm::vec3;
using vec4=glm::vec4;
using mat2=glm::mat2;
using mat3=glm::mat3;
using mat4=glm::mat4;
using uvec2=glm::uvec2;
using uvec3=glm::uvec3;
using uvec4=glm::uvec4;
using uint2 = uvec2;
using uint3 = uvec3;
using uint4 = uvec4;
using ivec2=glm::ivec2;
using ivec3=glm::ivec3;
using ivec4=glm::ivec4;
using int2 = ivec2;
using int3 = ivec3;
using int4 = ivec4;
#else
#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4
#define int2  ivec2
#define int3  ivec3
#define int4  ivec4
#include "transform.h"
#endif

// Frame
struct FrameInfo {
    uvec2 resolution;
    uint  frameID;
};

// Camera
struct CameraInfo {
    mat4 V;
    mat4 P;
    mat4 invVP;

    mat4 surfelGridFrustumPosX;
    mat4 surfelGridFrustumNegX;
    mat4 surfelGridFrustumPosY;
    mat4 surfelGridFrustumNegY;
    mat4 surfelGridFrustumPosZ;
    mat4 surfelGridFrustumNegZ;

    vec3 position;
    float zNear;
    float zFar;
    float zFarRcp;
};

// Lights
#define LIGHT_TYPE_POINT       0
#define LIGHT_TYPE_SPOTLIGHT   1
#define LIGHT_TYPE_DIRECTIONAL 2
struct LightInfo {
    vec3 position;
    float padding0;

    vec3 direction;
    float padding1;

    vec3 color;
    float padding2;

    float intensity;                    // light strength or intensity
    float range;                        // maximum range of light, 0 for infinity
    float decay;                        // the amount the light dims along the distance
    float angle;                        // spot-light opening angle

    float penumbra;                     // percent of spotlight cone attenuated due to penumbra.
    uint  type;
    uint  padding3;
    uint  padding4;
};

struct SkyInfo {
    vec3 color;
    float intensity;
};

// Materials
struct MaterialInfo {
    vec4 baseColor;
    vec4 emissiveColor;

    int baseColorTexture;
    int baseColorSampler;
    int emissiveTexture;
    int emissiveSampler;

    float metalness;
    float roughness;
    int metallicRoughnessTexture;
    int metallicRoughnessSampler;
};

// Instances
struct InstanceInfo {
    mat4     M;
    mat4     N;
    uint     instanceID;
    uint     materialID;
    uint64_t indexAddress;
    uint64_t vertexAddress;
    uint64_t materialAddress;
};

// Vertex
struct VertexType {
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 uv0;
    vec2 uv1;
    vec4 color0;
    vec4 joints0;
    vec4 weights0;
};

// Hit Info
struct HitInfo {
    vec3  position;
    vec3  normal;
    vec3  albedo;
    vec3  emissive;
    float distance;
};

// Surfels
const uint  SURFEL_CAPACITY         = 250000;
const uint  SURFEL_CAPACITY_SQRT    = 500;
const uint  SURFEL_TILE_X           = 16;
const uint  SURFEL_TILE_Y           = 16;
const uint  SURFEL_CELL_CAPACITY    = 0xffff;
const float SURFEL_GRID_SIZE        = 1.0;
const uint3 SURFEL_GRID_DIMS        = uint3(96, 96, 96);
const uint3 SURFEL_GRID_INNER_DIMS  = uint3(48, 48, 48);
const uint3 SURFEL_GRID_OUTER_DIMS  = SURFEL_GRID_DIMS - SURFEL_GRID_INNER_DIMS;
const uint  SURFEL_GRID_COUNT       = SURFEL_GRID_DIMS.x
                                    * SURFEL_GRID_DIMS.y
                                    * SURFEL_GRID_DIMS.z;
const float SURFEL_LOW_COVERAGE     = 1.0;
const float SURFEL_HIGH_COVERAGE    = 1.5;
const float SURFEL_MAX_RADIUS       = 1.0;
const float SURFEL_MIN_RADIUS       = 0.5;
const uint SURFEL_UPDATE_GROUP_SIZE = 32;
const uint MAX_RAYS_PER_SURFEL      = 32;

// record radial gaussian depth function values
const uint SURFEL_DEPTH_TEXELS          = 6;
const uint SURFEL_DEPTH_ATLAS_TEXELS    = SURFEL_DEPTH_TEXELS * SURFEL_CAPACITY_SQRT;

// record relative radiance for ray guiding
const uint SURFEL_RAYGUIDE_TEXELS       = 6;
const uint SURFEL_RAYGUIDE_ATLAS_TEXELS = SURFEL_RAYGUIDE_TEXELS * SURFEL_CAPACITY_SQRT;

struct Surfel {
    vec3  position;
    uint  normal;
    vec3  color;
    float radius;
};

struct SurfelData {
    vec3  position;
    uint  normal;

    vec3  mean;
    uint  instanceID;

    vec3  shortMean;
    uint  surfelID;

    vec3  variance;
    float rayGuideScale; // NOTE: 16b is enough

    float vbbr;
    float inconsistency;
    uint  life;
    uint  recycle;
};

struct SurfelStat {
    uint count;     // number of live surfels
    uint alloc;     // cell allocator
    uint pause;     // pause config
    uint x;
    uint y;
    uint z;
};

struct SurfelGridCell {
    uint count;
    uint offset;
};

#define SURFEL_DEBUG_POINT          1
#define SURFEL_DEBUG_RANDOM         2
#define SURFEL_DEBUG_RADIAL_DEPTH   3
#define SURFEL_DEBUG_NORMAL         4
#define SURFEL_DEBUG_INCONSISTENCY  5

struct DebugControl {
    uint showOverlay;
    uint showLight;
    uint showGrid;
    uint showSurfelInfo;
    uint showSampleRays;
    uint showDirectDiffuse;
    uint showGlobalDiffuse;
};

// Descriptor Set
#define SCENE_FRAME_BINDING         0
#define SCENE_CAMERA_BINDING        1
#define SCENE_LIGHT_BINDING         2
#define SCENE_INSTANCE_BINDING      3
#define SCENE_MATERIAL_BINDING      4
#define SCENE_ACCEL_BINDING         5
#define SCENE_SKY_BINDING           6
#define SCENE_LIGHT_XFORM_BINDING   7

// Descriptor Set
#define SCENE_IMAGES_BINDING        0

// Descriptor Set
#define SCENE_SAMPLERS_BINDING      0

// Descriptor Set
#define GBUFFER_ALBEDO_BINDING              0
#define GBUFFER_EMISSIVE_BINDING            1
#define GBUFFER_METALLIC_ROUGHNESS_BINDING  2
#define GBUFFER_NORMAL_BINDING              3
#define GBUFFER_DEPTH_BINDING               4
#define GBUFFER_OBJECT_BINDING              5
#define GBUFFER_GLOBAL_DIFFUSE_BINDING      6
#define GBUFFER_DIRECT_DIFFUSE_BINDING      7
#define GBUFFER_SPECULAR_BINDING            8
#define GBUFFER_POSITION_BINDING            9

// Descriptor Set
#define SURFEL_BINDING              0
#define SURFEL_LIVE_BINDING         1
#define SURFEL_DATA_BINDING         2
#define SURFEL_GRID_BINDING         3
#define SURFEL_CELL_BINDING         4
#define SURFEL_STAT_BINDING         5
#define SURFEL_COVERAGE_BINDING     7
#define SURFEL_DEPTH_BINDING        8
#define SURFEL_RAYGUIDE_BINDING     9
#define SURFEL_DEPTH_TEX_BINDING    10
#define SURFEL_RAYGUIDE_TEX_BINDING 11

// Descriptor Set
#define DEBUG_DEPTH_BINDING         0
#define DEBUG_OBJECT_BINDING        1
#define DEBUG_GRID_BINDING          2
#define DEBUG_SURFEL_BUDGET_BINDING 3
#define DEBUG_SURFEL_BINDING        4
#define DEBUG_SURFEL_VAR_BINDING    5
#define DEBUG_SURFEL_RAYDIR_BINDING 6

#define saturate(X) clamp(X, 0.0, 1.0)

#ifndef __cplusplus

// check if world position is in uniform grid
bool in_uniform_surfel_grid(in CameraInfo cam, vec3 worldPos) {
    vec3 grid = (worldPos / SURFEL_GRID_SIZE) - floor(cam.position / SURFEL_GRID_SIZE);

    // check for inner uniform grids
    int3 innerGrid = int3(floor(grid));
    int3 innerBound = int3(SURFEL_GRID_INNER_DIMS) / 2;
    bvec3 valid1 = lessThan(innerGrid, innerBound);
    bvec3 valid2 = greaterThanEqual(innerGrid, -innerBound);
    return all(valid1) && all(valid2);
}

// compute 3d grid index for a world position
// returns the 3d grid index
int3 compute_surfel_grid(in CameraInfo cam, vec3 worldPos) {
    vec3 grid = (worldPos / SURFEL_GRID_SIZE) - floor(cam.position / SURFEL_GRID_SIZE);

    // check for inner uniform grids
    int3 innerGrid = int3(floor(grid));
    int3 innerBound = int3(SURFEL_GRID_INNER_DIMS) / 2;
    bvec3 valid1 = lessThan(innerGrid, innerBound);
    bvec3 valid2 = greaterThanEqual(innerGrid, -innerBound);
    if (all(valid1) && all(valid2)) {
        return innerGrid;
    }

    // find major axis
    float maxDimValue = max(max(abs(grid.x), abs(grid.y)), abs(grid.z));

    if (maxDimValue == abs(grid.x)) {
        if (grid.x > 0) {
            vec4 tmp0 = cam.surfelGridFrustumPosX * vec4(worldPos, 1.0);
            vec3 tmp1 = tmp0.zyx / tmp0.w;
            int3 tmp2 = int3(floor(tmp1 * SURFEL_GRID_OUTER_DIMS / 2.0));
            return  tmp2 + int3(SURFEL_GRID_INNER_DIMS.x / 2, 0, 0);
        } else {
            vec4 tmp0 = cam.surfelGridFrustumNegX * vec4(worldPos, 1.0);
            vec3 tmp1 = tmp0.zyx / tmp0.w;
            int3 tmp2 = int3(floor(tmp1 * SURFEL_GRID_OUTER_DIMS / 2.0));
            return -tmp2 - int3(SURFEL_GRID_INNER_DIMS.x / 2, 0, 0);
        }
    }

    else if (maxDimValue == abs(grid.y)) {
        if (grid.y > 0) {
            vec4 tmp0 = cam.surfelGridFrustumPosY * vec4(worldPos, 1.0);
            vec3 tmp1 = tmp0.xzy / tmp0.w;
            int3 tmp2 = int3(floor(tmp1 * SURFEL_GRID_OUTER_DIMS / 2.0));
            return  tmp2 + int3(0, SURFEL_GRID_INNER_DIMS.y / 2, 0);
        } else {
            vec4 tmp0 = cam.surfelGridFrustumNegY * vec4(worldPos, 1.0);
            vec3 tmp1 = tmp0.zyx / tmp0.w;
            int3 tmp2 = int3(floor(tmp1 * SURFEL_GRID_OUTER_DIMS / 2.0));
            return -tmp2 - int3(0, SURFEL_GRID_INNER_DIMS.y / 2, 0);
        }
    }

    else if (maxDimValue == abs(grid.z)) {
        if (grid.z > 0) {
            vec4 tmp0 = cam.surfelGridFrustumPosZ * vec4(worldPos, 1.0);
            vec3 tmp1 = tmp0.xyz / tmp0.w;
            int3 tmp2 = int3(floor(tmp1 * SURFEL_GRID_OUTER_DIMS / 2.0));
            return  tmp2 + int3(0, 0, SURFEL_GRID_INNER_DIMS.z / 2);
        } else {
            vec4 tmp0 = cam.surfelGridFrustumNegZ * vec4(worldPos, 1.0);
            vec3 tmp1 = tmp0.xyz / tmp0.w;
            int3 tmp2 = int3(floor(tmp1 * SURFEL_GRID_OUTER_DIMS / 2.0));
            return -tmp2 - int3(0, 0, SURFEL_GRID_INNER_DIMS.z / 2);
        }
    }

    return int3(SURFEL_GRID_DIMS);
}

mat3 get_tangent_space(vec3 normal) {
    // choose a helper vector for the cross product
    vec3 helper = abs(normal.x) > 0.99 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);

    // generate vectors
    vec3 tangent = normalize(cross(normal, helper));
    vec3 binormal = normalize(cross(normal, tangent));
    return mat3(tangent, binormal, normal);
}

// compute the array index for surfel
// returns the 1d cell index
uint compute_surfel_cell(int3 gridIndex) {
    uint3 cellIndex = uint3(gridIndex) + SURFEL_GRID_DIMS / 2;
    return cellIndex.x * SURFEL_GRID_DIMS.y * SURFEL_GRID_DIMS.z
         + cellIndex.y * SURFEL_GRID_DIMS.z
         + cellIndex.z;
}

bool compute_surfel_grid_intersection(in vec3 camPos, in int3 grid, in vec3 surfelP, in vec3 surfelN, in float surfelR) {
    // compute world origin based on camera
    vec3 origin = floor(camPos / SURFEL_GRID_SIZE);

    // compute grid cell center
    vec3 cellCenter = origin + vec3(grid) * SURFEL_GRID_SIZE + vec3(SURFEL_GRID_SIZE) / 2.0;
    float gridCellRadius = SURFEL_GRID_SIZE * 0.5;

    // compute surfel's local pos in cell's coordinate frame
    vec3 cellLocalSurfelPos = surfelP - cellCenter;
    vec3 cellLocalClosestPointOnGridCell = clamp(cellLocalSurfelPos, vec3(-gridCellRadius), vec3(+gridCellRadius));
    vec3 posOffset = cellLocalSurfelPos - cellLocalClosestPointOnGridCell;

#if 1
    // Approximate box-ellipsoid culling.
    // Sometimes misses corners, but greatly improves culling efficiency.
    const float SURFEL_NORMAL_DIRECTION_SQUISH = 2.0;
    float mahalanobisDist = length(posOffset) * (1.0 + abs(dot(posOffset, surfelN)) * SURFEL_NORMAL_DIRECTION_SQUISH);
    return mahalanobisDist < surfelR;
#else
    return dot(posOffset, posOffset) < (surfelR * surfelR);
#endif
}

// compute 1d index given dimension and 2d index
uint flatten_2d(uint2 coord, uint2 dim) {
    return coord.x + coord.y * dim.x;
}

// compute 2d index given dimension and 1d index
uint2 unflatten_2d(uint index, uint2 dim) {
    return uint2(index % dim.x, index / dim.x);
}

float luminance(const vec3 color) {
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

float signNotZero(float v) {
    return v >= 0.0 ? 1.0 : -1.0;
}

vec2 signNotZero(vec2 v) {
    return vec2(signNotZero(v.x), signNotZero(v.y));
}

// encode hemisphere into octahedron
vec2 encode_hemioct(vec3 v) {
    // project hemisphere onto hemi-octahedron
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + v.z));
    // rotate and scale the center diamon to the unit square
    return vec2(p.x + p.y, p.x - p.y);
}

// decode octahedron to hemisphere
vec3 decode_hemioct(vec2 e) {
    // rotate and scale the unit square back to the center diamond
    vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
    vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));
    return normalize(v);
}

// result in [-1, 1]^2
vec2 encode_oct(vec3 v) {
    vec2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    // reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

// result in [-1, +1]^3
vec3 decode_oct(vec2 e) {
    vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    return normalize(v);
}

// compute pixel coordinate on surfel depth octmap
vec2 compute_surfel_depth_pixel(uint surfelIndex, vec3 direction, vec3 normal) {
    uint2 pixel = unflatten_2d(surfelIndex, uint2(SURFEL_CAPACITY_SQRT)) * SURFEL_DEPTH_TEXELS;

    // NOTE: direction is in world space,
    // we need to transform it to z-axis oriented hemisphere
    vec4 q = compute_rotation_to_z_axis(normal);
    vec3 hemi = rotate_point(q, direction);
    hemi.z = abs(hemi.z);
    vec2 uv = encode_hemioct(hemi) * 0.5 + 0.5;

    uv = max(min(uv, vec2(0.99)), vec2(0.00));
    return pixel + uv * SURFEL_DEPTH_TEXELS;
}

// compute pixel coordinate on surfel rayguide octmap
vec2 compute_surfel_rayguide_pixel(uint surfelIndex, vec3 direction, vec3 normal) {
    uint2 pixel = unflatten_2d(surfelIndex, uint2(SURFEL_CAPACITY_SQRT)) * SURFEL_RAYGUIDE_TEXELS;

    // NOTE: direction is in world space,
    // we need to transform it to z-axis oriented hemisphere
    vec4 q = compute_rotation_to_z_axis(normal);
    vec3 hemi = rotate_point(q, direction);
    hemi.z = abs(hemi.z);
    vec2 uv = encode_hemioct(hemi) * 0.5 + 0.5;

    uv = max(min(uv, vec2(0.99)), vec2(0.00));
    return pixel + uv * SURFEL_RAYGUIDE_TEXELS;
}

// compute surfel depth on hemisphere
float compute_surfel_depth_weight(vec2 weight, float dist) {
    float mean = weight.x;
    float mean2 = weight.y;
    if (dist > mean) {
        // chebyshev
        float variance = abs(mean * mean - mean2);
        float diff = dist - mean;
        return max(0.0, pow(variance / (variance + diff * diff), 6.0));
    }
    return 1.0;
}


// adaptive normal offset
vec3 offset_ray_origin(const vec3 P, const vec3 N) {
    const float origin = 1.0f / 32.0f;
    const float intScale = 256.0f;
    const float floatScale = 1.0 / 65536.0f;

    const ivec3 iOffset = ivec3(intScale * N);
    const vec3 iP = vec3(
        intBitsToFloat(floatBitsToInt(P.x) + ((P.x < 0.0) ? -iOffset.x : iOffset.x)),
        intBitsToFloat(floatBitsToInt(P.y) + ((P.y < 0.0) ? -iOffset.y : iOffset.y)),
        intBitsToFloat(floatBitsToInt(P.z) + ((P.z < 0.0) ? -iOffset.z : iOffset.z)));

    return vec3(
        abs(P.x) < origin ? P.x + floatScale * N.x : iP.x,
        abs(P.y) < origin ? P.y + floatScale * N.y : iP.y,
        abs(P.z) < origin ? P.z + floatScale * N.z : iP.z);
}

#endif

#endif
