#ifndef SLIM_UTILITY_GEOMETRY_H
#define SLIM_UTILITY_GEOMETRY_H

#include <vector>
#include <glm/glm.hpp>
#include "utility/interface.h"
#include "utility/boundingbox.h"

namespace slim {

    struct GeometryData {

        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texcoord;
        };

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Plane {
        float    width          = 1.0f;
        float    height         = 1.0f;
        uint32_t widthSegments  = 1;
        uint32_t heightSegments = 1;
        bool     ccw            = true;

        GeometryData Create() const;
    };

    struct Cube {
        float    width          = 1.0f;
        float    height         = 1.0f;
        float    depth          = 1.0f;
        uint32_t widthSegments  = 1;
        uint32_t heightSegments = 1;
        uint32_t depthSegments  = 1;
        bool     ccw            = true;

        GeometryData Create() const;
    };

    struct Sphere {
        float    radius         = 1.0f;
        uint32_t radialSegments = 32;
        uint32_t heightSegments = 32;
        float    phiStart       = 0.0f;         // vertical starting angle
        float    phiLength      = M_PI;         // vertical sweep angle
        float    thetaStart     = 0.0f;         // horizontal starting angle
        float    thetaLength    = M_PI * 2.0f;  // horizontal sweep angle
        bool     ccw            = true;

        GeometryData Create() const;
    };

    struct Cone {
        float    radius         = 1.0f;
        float    height         = 1.0f;
        uint32_t radialSegments = 8;
        uint32_t heightSegments = 1;
        float    thetaStart     = 0.0f;
        float    thetaLength    = M_PI * 2.0f;
        bool     openEnded      = false;
        bool     ccw            = true;

        GeometryData Create() const;
    };

    struct Cylinder {
        float radiusTop = 1.0f;
        float radiusBottom = 1.0f;
        float height = 1.0f;
        uint32_t radialSegments = 8;
        uint32_t heightSegments = 1;
        float    thetaStart     = 0.0f;
        float    thetaLength    = M_PI;
        bool     openEnded      = false;
        bool     ccw            = true;

        GeometryData Create() const;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_GEOMETRY_H
