// https://www.ronja-tutorials.com/post/025-value-noise/
// with functions renamed and re-organized

// Unlike white noise, value noise provides smooth change for the values.
// The way to implement value noise is to interpolate between noise values.

#ifndef SLIM_NOISE_LIB_VALUE_NOISE
#define SLIM_NOISE_LIB_VALUE_NOISE

#include "white.hlsli"

namespace noises {
namespace value {

    // to 1d functions

    // get a scalar random value from an 1d value
    float rand1d(float value, float mutator) {
        // get values of cell based on interpolation
        float  cellL    = noises::white::rand1d(floor(value), mutator);
        float  cellR    = noises::white::rand1d( ceil(value), mutator);
        // perform interpolation
        float  fraction = frac(value);
        float  t        = smoothstep(0.0, 1.0, fraction);
        float  cell     = lerp(cellL, cellR, t);
        return cell;
    }

    // get a scalar random value from a 2d value
    float rand1d(float2 value, float2 mutator = float2(12.9898, 78.233)) {
        // get values of cell based on interpolation
        float  cellTL   = noises::white::rand1d(float2(floor(value.x),  ceil(value.y)), mutator);
        float  cellTR   = noises::white::rand1d(float2( ceil(value.x),  ceil(value.y)), mutator);
        float  cellBL   = noises::white::rand1d(float2(floor(value.x), floor(value.y)), mutator);
        float  cellBR   = noises::white::rand1d(float2( ceil(value.x), floor(value.y)), mutator);
        // perform interpolation
        float2 fraction = frac(value);
        float2 t        = smoothstep(0.0, 1.0, fraction);
        float  cellT    = lerp(cellTL, cellTR, t.x);
        float  cellB    = lerp(cellBL, cellBR, t.x);
        float  cell     = lerp(cellB,  cellT,  t.y);
        return cell;
    }

    // get a scalar random value from a 3d value
    float rand1d(float3 value, float3 mutator = float3(12.9898, 78.233, 37.719)) {
        // get values of cell based on interpolation
        float  cellBTL  = noises::white::rand1d(float3(floor(value.x),  ceil(value.y), floor(value.z)), mutator);
        float  cellBTR  = noises::white::rand1d(float3( ceil(value.x),  ceil(value.y), floor(value.z)), mutator);
        float  cellFTL  = noises::white::rand1d(float3(floor(value.x),  ceil(value.y),  ceil(value.z)), mutator);
        float  cellFTR  = noises::white::rand1d(float3( ceil(value.x),  ceil(value.y),  ceil(value.z)), mutator);
        float  cellBBL  = noises::white::rand1d(float3(floor(value.x), floor(value.y), floor(value.z)), mutator);
        float  cellBBR  = noises::white::rand1d(float3( ceil(value.x), floor(value.y), floor(value.z)), mutator);
        float  cellFBL  = noises::white::rand1d(float3(floor(value.x), floor(value.y),  ceil(value.z)), mutator);
        float  cellFBR  = noises::white::rand1d(float3( ceil(value.x), floor(value.y),  ceil(value.z)), mutator);
        // perform interpolation
        float3 fraction = frac(value);
        float3 t        = smoothstep(0.0, 1.0, fraction);
        float  cellBT   = lerp(cellBTL, cellBTR, t.x);
        float  cellBB   = lerp(cellBBL, cellBBR, t.x);
        float  cellFT   = lerp(cellFTL, cellFTR, t.x);
        float  cellFB   = lerp(cellFBL, cellFBR, t.x);
        float  cellB    = lerp(cellBB,  cellBT,  t.y);
        float  cellF    = lerp(cellFB,  cellFT,  t.y);
        float  cell     = lerp(cellB,   cellF,   t.z);
        return cell;
    }

    // to 2d functions

    // get a 2d vector from an 1d value
    float2 rand2d(float value, float mutator = 0) {
        return float2(
            noises::value::rand1d(value + mutator, 3.9812),
            noises::value::rand1d(value + mutator, 7.1536)
        );
    }

    // get a 2d vector from an 2d value
    float2 rand2d(float2 value, float2 mutator = float2(0, 0)) {
        return float2(
            noises::value::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::value::rand1d(value + mutator, float2(39.346, 11.135))
        );
    }

    // get a 2d vector from an 3d value
    float2 rand2d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float2(
            noises::value::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::value::rand1d(value + mutator, float3(39.346, 11.135, 83.155))
        );
    }

    //to 3d functions

    // get a 3d value from an 1d value
    float3 rand3d(float value, float mutator = 0.0) {
        return float3(
            noises::value::rand1d(value + mutator, 3.9812),
            noises::value::rand1d(value + mutator, 7.1536),
            noises::value::rand1d(value + mutator, 5.7241)
        );
    }

    // get a 3d value from a 2d value
    float3 rand3d(float2 value, float2 mutator = float2(0, 0)) {
        return float3(
            noises::value::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::value::rand1d(value + mutator, float2(39.346, 11.135)),
            noises::value::rand1d(value + mutator, float2(73.156, 52.235))
        );
    }

    // get a 3d value from a 3d value
    float3 rand3d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float3(
            noises::value::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::value::rand1d(value + mutator, float3(39.346, 11.135, 83.155)),
            noises::value::rand1d(value + mutator, float3(73.156, 52.235, 09.151))
        );
    }

} // end of namespace value
} // end of namespace noises

#endif // end of SLIM_NOISE_LIB_VALUE_NOISE
