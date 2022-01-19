// https://www.ronja-tutorials.com/post/025-value-noise/
// with functions renamed and re-organized

#ifndef SLIM_NOISE_LIB_VORONOI_NOISE
#define SLIM_NOISE_LIB_VORONOI_NOISE

#include "white.hlsli"

namespace noises {
namespace voronoi {

    // to 1d functions

    // get a scalar random value from an 1d value
    float rand1d(float value,
                 out float nearestCell,
                 out float minEdgeDist,
                 float mutator = 0.0) {
        float baseCell = floor(value);

        // First pass to find closest cell
        float minDistToCell = 100.0;
        float toNearestCell;
        [unroll]
        for (int x = -1; x <= 1; x++) {
            float cell = baseCell + x;
            float randPosition = noises::white::rand1d(cell, mutator) * 0.5 + 0.5;
            float cellPosition = cell + randPosition;
            float toCell = cellPosition - value;
            float distToCell = length(toCell);
            if (distToCell < minDistToCell) {
                minDistToCell = distToCell;
                nearestCell = cell;
                toNearestCell = toCell;
            }
        }

        // Second pass to find closest edge
        [unroll]
        minEdgeDist = 100.0;
        for (int x = -1; x <= 1; x++) {
            float cell = baseCell + x;
            float randPosition = noises::white::rand2d(cell, mutator) * 0.5 + 0.5;
            float cellPosition = cell + randPosition;
            float toCell = cellPosition - value;
            float diffToNearestCell = abs(nearestCell - cell);
            bool isNearestCell = diffToNearestCell < 0.1;
            if (!isNearestCell) {
                float toCenter = (toNearestCell + toCell) / 2.0;
                minEdgeDist = min(toCenter, minEdgeDist);
            }
        }

        return minDistToCell * 2.0 - 1.0;
    }

    // get a scalar random value from a 2d value
    float rand1d(float2 value,
                 out float2 nearestCell,
                 out float minEdgeDist,
                 float2 mutator = float2(0, 0)) {
        float2 baseCell = floor(value);

        // First pass to find closest cell
        float minDistToCell = 100.0;
        float2 toNearestCell;
        [unroll]
        for (int x = -1; x <= 1; x++) {
            [unroll]
            for (int y = -1; y <= 1; y++) {
                float2 cell = baseCell + float2(x, y);
                float2 randPosition = noises::white::rand2d(cell, mutator) * 0.5 + 0.5;
                float2 cellPosition = cell + randPosition;
                float2 toCell = cellPosition - value;
                float distToCell = length(toCell);
                if (distToCell < minDistToCell) {
                    minDistToCell = distToCell;
                    nearestCell = cell;
                    toNearestCell = toCell;
                }
            }
        }

        // Second pass to find closest edge
        [unroll]
        minEdgeDist = 100.0;
        for (int x = -1; x <= 1; x++) {
            [unroll]
            for (int y = -1; y <= 1; y++) {
                float2 cell = baseCell + float2(x, y);
                float2 randPosition = noises::white::rand2d(cell, mutator) * 0.5 + 0.5;
                float2 cellPosition = cell + randPosition;
                float2 toCell = cellPosition - value;
                float2 diffToNearestCell = abs(nearestCell - cell);
                bool isNearestCell = (diffToNearestCell.x + diffToNearestCell.y) < 0.1;
                if (!isNearestCell) {
                    float2 toCenter = (toNearestCell + toCell) / 2.0;
                    float2 cellDifference = normalize(toCell - toNearestCell);
                    float edgeDistance = dot(toCenter, cellDifference);
                    minEdgeDist = min(edgeDistance, minEdgeDist);
                }
            }
        }

        return minDistToCell * 2.0 - 1.0;
    }

    // get a scalar random value from a 2d value
    float rand1d(float3 value,
                 out float3 nearestCell,
                 out float minEdgeDist,
                 float3 mutator = float3(0, 0, 0)) {
        float3 baseCell = floor(value);

        // First pass to find closest cell
        float minDistToCell = 100.0;
        float3 toNearestCell;
        [unroll]
        for (int x = -1; x <= 1; x++) {
            [unroll]
            for (int y = -1; y <= 1; y++) {
                [unroll]
                for (int z = -1; z <= 1; z++) {
                    float3 cell = baseCell + float3(x, y, z);
                    float3 randPosition = noises::white::rand3d(cell, mutator) * 0.5 + 0.5;
                    float3 cellPosition = cell + randPosition;
                    float3 toCell = cellPosition - value;
                    float distToCell = length(toCell);
                    if (distToCell < minDistToCell) {
                        minDistToCell = distToCell;
                        nearestCell = cell;
                        toNearestCell = toCell;
                    }
                }
            }
        }

        // Second pass to find closest edge
        [unroll]
        minEdgeDist = 100.0;
        for (int x = -1; x <= 1; x++) {
            [unroll]
            for (int y = -1; y <= 1; y++) {
                [unroll]
                for (int z = -1; z <= 1; z++) {
                    float3 cell = baseCell + float3(x, y, z);
                    float3 randPosition = noises::white::rand3d(cell, mutator) * 0.5 + 0.5;
                    float3 cellPosition = cell + randPosition;
                    float3 toCell = cellPosition - value;
                    float3 diffToNearestCell = abs(nearestCell - cell);
                    bool isNearestCell = (diffToNearestCell.x + diffToNearestCell.y + diffToNearestCell.z) < 0.1;
                    if (!isNearestCell) {
                        float3 toCenter = (toNearestCell + toCell) / 2.0;
                        float3 cellDifference = normalize(toCell - toNearestCell);
                        float edgeDistance = dot(toCenter, cellDifference);
                        minEdgeDist = min(edgeDistance, minEdgeDist);
                    }
                }
            }
        }

        return minDistToCell * 2.0 - 1.0;
    }

} // end of namespace value
} // end of namespace noises

#endif // end of SLIM_NOISE_LIB_VORONOI_NOISE
