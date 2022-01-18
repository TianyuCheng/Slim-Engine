// https://www.ronja-tutorials.com/post/024-white-noise/
// with functions renamed and re-organized

#ifndef SLIM_NOISE_LIB_WHITE_NOISE
#define SLIM_NOISE_LIB_WHITE_NOISE

namespace noises {
namespace white {

    // to 1d functions

    // get a scalar random value from an 1d value
    float rand1d(float value, float mutator = 0.546) {
        float random = frac(sin(value + mutator) * 143758.5453);
        return random * 2.0 - 1.0;
    }

    // get a scalar random value from a 2d value
    float rand1d(float2 value, float2 dotDir = float2(12.9898, 78.233)) {
        float2 smallValue = sin(value);
        float random = dot(smallValue, dotDir);
        random = frac(sin(random) * 143758.5453);
        return random * 2.0 - 1.0;
    }

    // get a scalar random value from a 3d value
    float rand1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719)) {
        //make value smaller to avoid artifacts
        float3 smallValue = sin(value);
        //get scalar value from 3d vector
        float random = dot(smallValue, dotDir);
        //make value more random by making it bigger and then taking the factional part
        random = frac(sin(random) * 143758.5453);
        return random * 2.0 - 1.0;
    }

    // to 2d functions

    // get a 2d vector from an 1d value
    float2 rand2d(float value, float mutator = 0.0) {
        return float2(
            noises::white::rand1d(value + mutator, 3.9812),
            noises::white::rand1d(value + mutator, 7.1536)
        );
    }

    // get a 2d vector from an 2d value
    float2 rand2d(float2 value, float2 mutator = float2(0, 0)) {
        return float2(
            noises::white::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::white::rand1d(value + mutator, float2(39.346, 11.135))
        );
    }

    // get a 2d vector from an 3d value
    float2 rand2d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float2(
            noises::white::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::white::rand1d(value + mutator, float3(39.346, 11.135, 83.155))
        );
    }

    //to 3d functions

    // get a 3d value from an 1d value
    float3 rand3d(float value, float mutator = 0.0) {
        return float3(
            noises::white::rand1d(value + mutator, 3.9812),
            noises::white::rand1d(value + mutator, 7.1536),
            noises::white::rand1d(value + mutator, 5.7241)
        );
    }

    // get a 3d value from a 2d value
    float3 rand3d(float2 value, float2 mutator = float2(0, 0)) {
        return float3(
            noises::white::rand1d(value + mutator, float2(12.989, 78.233)),
            noises::white::rand1d(value + mutator, float2(39.346, 11.135)),
            noises::white::rand1d(value + mutator, float2(73.156, 52.235))
        );
    }

    // get a 3d value from a 3d value
    float3 rand3d(float3 value, float3 mutator = float3(0, 0, 0)) {
        return float3(
            noises::white::rand1d(value + mutator, float3(12.989, 78.233, 37.719)),
            noises::white::rand1d(value + mutator, float3(39.346, 11.135, 83.155)),
            noises::white::rand1d(value + mutator, float3(73.156, 52.235, 09.151))
        );
    }

} // end of namespace white
} // end of namespace noises

#endif // end of SLIM_NOISE_LIB_WHITE_NOISE
