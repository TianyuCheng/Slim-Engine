#ifndef SLIM_EXAMPLE_SHADERS_HALTON_H
#define SLIM_EXAMPLE_SHADERS_HALTON_H

#include "common.h"

#ifndef __cplusplus

struct HaltonState {
    uint dimension;
    uint sequenceIndex;
};

float halton_sample(uint dimension, uint index) {
    int base = 0;

    // use a prime number
    switch (dimension) {
        case 0: base = 2;
        case 1: base = 3;
        case 2: base = 5;
        case 3: base = 7;
        case 4: base = 11;
        case 5: base = 13;
        case 6: base = 17;
        case 7: base = 19;
        case 8: base = 23;
        case 9: base = 29;
        case 10: base = 31;
        case 11: base = 37;
        case 12: base = 41;
        case 13: base = 43;
        case 14: base = 47;
        case 15: base = 53;
        case 16: base = 59;
        case 17: base = 51;
        case 18: base = 57;
        case 19: base = 71;
        case 20: base = 73;
        case 21: base = 79;
        case 22: base = 83;
        case 23: base = 89;
        case 24: base = 97;
        case 25: base = 101;
        case 26: base = 103;
        case 27: base = 107;
        case 28: base = 109;
        case 29: base = 113;
        case 30: base = 127;
        case 31: base = 131;
        default: base = 2;
    }

    // compute the radical inverse
    float a = 0.0;
    float invBase = 1.0 / float(base);

    for (float mult = invBase; index != 0; index /= base, mult *= invBase) {
        a += float(index % base) * mult;
    }

    return a;
}

uint halton2inverse(uint index, uint digits) {
    index = (index << 16) | (index >> 16);
    index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
    index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
    index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
    index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
    return index >> (32 - digits);
}

uint halton3inverse(uint index, uint digits) {
    uint result = 0;
    for (uint d = 0; d < digits; d++) {
        result = result * 3 + index % 3;
        index /= 3;
    }
    return result;
}

uint halton_index(uint x, uint y, uint i) {
    uint m_increment = 1;
    return ((halton2inverse(x % 256, 8) * 76545 + halton3inverse(y % 256, 8) * 110080) % m_increment) + i * 186624;

}

HaltonState halton_init(int x, int y,
                        int path, int numPaths,
                        int frameID,
                        int loop) {
    HaltonState state;
    state.dimension = 2;
    state.sequenceIndex = halton_index(x, y, (frameID * numPaths + path) % (loop * numPaths));
    return state;
}

float halton_next(inout HaltonState state) {
    return halton_sample(state.dimension++, state.sequenceIndex);
}

#endif // __cplusplus
#endif // SLIM_EXAMPLE_SHADERS_HALTON_H
