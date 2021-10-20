#ifndef SLIM_SHADER_LIB_RANDOM_H
#define SLIM_SHADER_LIB_RANDOM_H

#include "glsl.hpp"

// PCG random number generator.
#define USE_PCG 1

#ifndef __cplusplus

uvec4 pcg4d(inout uvec4 v) {
    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    v = v ^ (v >> 16);

    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;

    return v;
}

uint xor_shift(inout uint rng) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

uint jenkins_hash(uint x) {
    x += x << 10;
    x ^= x >> 6;
    x += x << 3;
    x ^= x >> 11;
    x += x << 15;
    return x;
}

float uint_to_float(uint x) {
    return uintBitsToFloat(0x3f800000 | (x >> 9)) - 1.0;
}

#if USE_PCG
#define RNGState uvec4

RNGState init_rng(uvec2 pixel_coords, uvec2 resolution, uint frame) {
    // seed forPCG uses a sequential sample number in 4th channel,
    // which increments on every RNG call and starts from 0
    return RNGState(pixel_coords.xy, frame, 0);
}

float rand(inout RNGState rng) {
    rng.w++;   // increment sample index
    return uint_to_float(pcg4d(rng).x);
}

#else

#define RNGState uint

RNGState init_rng(uvec2 pixel_coords, uvec2 resolution, uint frame) {
    RNGState seed = (pixel_coords.x + pixel_coords.y * resolution.x) ^ jenkins_hash(frame);
    return jenkins_hash(seed);
}

float rand(inout RNGState rng) {
    return uint_to_float(xor_shift(rng));
}

#endif


#endif // __cplusplus
#endif // SLIM_SHADER_LIB_RANDOM_H
