#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "camera.h"
#include "surfel.h"

layout(push_constant) uniform FrameInfo {
    uvec2 size;
} info;

// camera information
layout(set = 0, binding = 0) uniform Camera {
    mat4 invVP; // inverse(P * V) for world position reconstruction
    vec3 pos;
    float zNear;
    float zFar;
    float zFarRcp;
} camera;

layout(set = 0, binding = 1) uniform sampler2D imageDepth;
layout(set = 0, binding = 2) uniform sampler2D imagePosition;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 ndc = (gl_FragCoord.xy + 0.5) / vec2(info.size);
    ivec2 pixel = ivec2(ndc * info.size);
    float depth = texture(imageDepth, inUV).x;
    vec3 pos = compute_world_position(ndc, depth, camera.invVP);

    ivec3 grid = compute_surfel_grid(pos, camera.pos);
    if (is_surfel_grid_valid(grid)) {
        ivec3 center = ivec3(floor(camera.pos)) + ivec3(SURFEL_GRID_DIMENSIONS / 2);
        int grid_index = center.x + center.y + center.z + grid.x + grid.y + grid.z;
        if (grid_index % 2 == 0) {
            outColor = vec4(0.5, 0.5, 0.5, 1.0);
        } else {
            outColor = vec4(0.9, 0.9, 0.9, 1.0);
        }
    } else {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
}
