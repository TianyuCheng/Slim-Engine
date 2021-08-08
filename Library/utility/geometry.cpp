#include <iostream>
#include "utility/geometry.h"

using namespace slim;

void CreatePlane(std::vector<GeometryData::Vertex>& vertices, std::vector<uint32_t>& indices,
                 const glm::vec3& x00, const glm::vec3& x01,
                 const glm::vec3& x10, const glm::vec3& x11,
                 const glm::vec3& normal,
                 uint32_t widthSegments, uint32_t heightSegments, bool ccw) {

    size_t indexOffset = indices.size();
    size_t vertexOffset = vertices.size();
    size_t vertexCount = (widthSegments + 1) * (heightSegments + 1);
    size_t indexCount = widthSegments * heightSegments * 2 * 3;
    vertices.reserve(vertexOffset + vertexCount);
    indices.reserve(indexOffset + indexCount);

    // vertices
    for (uint32_t i = 0; i <= widthSegments; i++) {
        for (uint32_t j = 0; j <= heightSegments; j++) {
            // texcoord
            float xUV = float(i) / float(widthSegments);
            float yUV = float(j) / float(heightSegments);
            // position
            glm::vec3 pos = glm::mix(glm::mix(x00, x01, xUV), glm::mix(x10, x11, xUV), yUV);
            // vertex
            vertices.push_back(GeometryData::Vertex { pos, normal, glm::vec2(xUV, yUV) });
        }
    }

    // indices
    uint32_t col = heightSegments + 1;
    for (uint32_t i = 0; i < widthSegments; i++) {
        for (uint32_t j = 0; j < heightSegments; j++) {
            uint32_t index = i * col + j;
            if (ccw) {
                indices.push_back(vertexOffset + index);
                indices.push_back(vertexOffset + index + 1);
                indices.push_back(vertexOffset + index + col);
                indices.push_back(vertexOffset + index + 1);
                indices.push_back(vertexOffset + index + 1 + col);
                indices.push_back(vertexOffset + index + col);
            } else {
                indices.push_back(vertexOffset + index);
                indices.push_back(vertexOffset + index + col);
                indices.push_back(vertexOffset + index + 1);
                indices.push_back(vertexOffset + index + 1);
                indices.push_back(vertexOffset + index + col);
                indices.push_back(vertexOffset + index + 1 + col);
            }
        }
    }
}

GeometryData Plane::Create() const {
    std::vector<GeometryData::Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    // single plane
    const glm::vec3 x00 = glm::vec3(-width / 2.0, 0.0, -height / 2.0);
    const glm::vec3 x01 = glm::vec3(+width / 2.0, 0.0, -height / 2.0);
    const glm::vec3 x10 = glm::vec3(-width / 2.0, 0.0, +height / 2.0);
    const glm::vec3 x11 = glm::vec3(+width / 2.0, 0.0, +height / 2.0);
    const glm::vec3 normal = ccw ? glm::vec3(0.0, 1.0, 0.0) : glm::vec3(0.0, -1.0, 0.0);
    CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);

    return GeometryData { vertices, indices };
}

GeometryData Cube::Create() const {
    std::vector<GeometryData::Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    // front plane
    {
        const glm::vec3 x00 = glm::vec3(-width / 2.0, -height / 2.0, +depth / 2.0);
        const glm::vec3 x01 = glm::vec3(+width / 2.0, -height / 2.0, +depth / 2.0);
        const glm::vec3 x10 = glm::vec3(-width / 2.0, +height / 2.0, +depth / 2.0);
        const glm::vec3 x11 = glm::vec3(+width / 2.0, +height / 2.0, +depth / 2.0);
        const glm::vec3 normal = ccw ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(0.0, 0.0, -1.0);
        CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);
    }

    // back plane
    {
        const glm::vec3 x00 = glm::vec3(-width / 2.0, -height / 2.0, -depth / 2.0);
        const glm::vec3 x01 = glm::vec3(+width / 2.0, -height / 2.0, -depth / 2.0);
        const glm::vec3 x10 = glm::vec3(-width / 2.0, +height / 2.0, -depth / 2.0);
        const glm::vec3 x11 = glm::vec3(+width / 2.0, +height / 2.0, -depth / 2.0);
        const glm::vec3 normal = ccw ? glm::vec3(0.0, 0.0, -1.0) : glm::vec3(0.0, 0.0, 1.0);
        CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);
    }

    // left plane
    {
        const glm::vec3 x00 = glm::vec3(-width / 2.0, -height / 2.0, -depth / 2.0);
        const glm::vec3 x01 = glm::vec3(-width / 2.0, -height / 2.0, +depth / 2.0);
        const glm::vec3 x10 = glm::vec3(-width / 2.0, +height / 2.0, -depth / 2.0);
        const glm::vec3 x11 = glm::vec3(-width / 2.0, +height / 2.0, +depth / 2.0);
        const glm::vec3 normal = ccw ? glm::vec3(-1.0, 0.0, 0.0) : glm::vec3(1.0, 0.0, 0.0);
        CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);
    }

    // right plane
    {
        const glm::vec3 x00 = glm::vec3(+width / 2.0, -height / 2.0, -depth / 2.0);
        const glm::vec3 x01 = glm::vec3(+width / 2.0, -height / 2.0, +depth / 2.0);
        const glm::vec3 x10 = glm::vec3(+width / 2.0, +height / 2.0, -depth / 2.0);
        const glm::vec3 x11 = glm::vec3(+width / 2.0, +height / 2.0, +depth / 2.0);
        const glm::vec3 normal = ccw ? glm::vec3(1.0, 0.0, 0.0) : glm::vec3(-1.0, 0.0, 0.0);
        CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);
    }

    // top plane
    {
        const glm::vec3 x00 = glm::vec3(-width / 2.0, +height / 2.0, -depth / 2.0);
        const glm::vec3 x01 = glm::vec3(+width / 2.0, +height / 2.0, -depth / 2.0);
        const glm::vec3 x10 = glm::vec3(-width / 2.0, +height / 2.0, +depth / 2.0);
        const glm::vec3 x11 = glm::vec3(+width / 2.0, +height / 2.0, +depth / 2.0);
        const glm::vec3 normal = ccw ? glm::vec3(0.0, 1.0, 0.0) : glm::vec3(0.0, -1.0, 0.0);
        CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);
    }

    // bottom plane
    {
        const glm::vec3 x00 = glm::vec3(-width / 2.0, -height / 2.0, -depth / 2.0);
        const glm::vec3 x01 = glm::vec3(+width / 2.0, -height / 2.0, -depth / 2.0);
        const glm::vec3 x10 = glm::vec3(-width / 2.0, -height / 2.0, +depth / 2.0);
        const glm::vec3 x11 = glm::vec3(+width / 2.0, -height / 2.0, +depth / 2.0);
        const glm::vec3 normal = ccw ? glm::vec3(0.0, -1.0, 0.0) : glm::vec3(0.0, +1.0, 0.0);
        CreatePlane(vertices, indices, x00, x01, x10, x11, normal, widthSegments, heightSegments, ccw);
    }

    return GeometryData { vertices, indices };
}

GeometryData Sphere::Create() const {
    std::vector<GeometryData::Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    size_t vertexCount = (radialSegments + 1) * (heightSegments + 1);
    size_t indexCount = radialSegments * heightSegments * 2 * 3;
    vertices.reserve(vertexCount);
    indices.reserve(indexCount);

    float phiEnd = phiStart + phiLength;
    float thetaEnd = thetaStart + thetaLength;

    // vertices
    for (uint32_t i = 0; i <= heightSegments; i++) {
        for (uint32_t j = 0; j <= radialSegments; j++) {
            float u = float(i) / float(heightSegments);
            float v = float(j) / float(radialSegments);
            float phi = glm::mix(phiStart, phiEnd, u);
            float theta = glm::mix(thetaStart, thetaEnd, v);
            glm::vec3 normal = glm::vec3(
                std::sin(phi) * std::cos(theta),
                std::cos(phi),
                std::sin(phi) * std::sin(theta));
            vertices.push_back(GeometryData::Vertex {
                radius * normal,
                normal,
                glm::vec2(u, v)
            });
        }
    }

    // indices
    uint32_t col = radialSegments + 1;
    for (uint32_t i = 0; i < heightSegments; i++) {
        for (uint32_t j = 0; j < radialSegments; j++) {
            uint32_t index = i * col + j;
            if (ccw) {
                indices.push_back(index);
                indices.push_back(index + 1);
                indices.push_back(index + col);
                indices.push_back(index + 1);
                indices.push_back(index + 1 + col);
                indices.push_back(index + col);
            } else {
                indices.push_back(index);
                indices.push_back(index + col);
                indices.push_back(index + 1);
                indices.push_back(index + 1);
                indices.push_back(index + col);
                indices.push_back(index + 1 + col);
            }
        }
    }

    return GeometryData { vertices, indices };
}

GeometryData Cone::Create() const {
    std::vector<GeometryData::Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    size_t vertexCount = (radialSegments + 1) * (heightSegments + 1);
    size_t indexCount = radialSegments * heightSegments * 2 * 3;
    vertices.reserve(vertexCount);
    indices.reserve(indexCount);

    float thetaEnd = thetaStart + thetaLength;

    // vertices
    for (uint32_t i = 0; i <= heightSegments; i++) {
        for (uint32_t j = 0; j <= radialSegments; j++) {
            float u = float(i) / float(heightSegments);
            float v = float(j) / float(radialSegments);
            float r = glm::mix(radius, 0.0f, u);
            float theta = glm::mix(thetaStart, thetaEnd, v);
            glm::vec3 pos = glm::vec3(
                r * std::cos(theta),
                height * u,
                r * std::sin(theta));
            vertices.push_back(GeometryData::Vertex {
                pos,
                glm::normalize(pos),
                glm::vec2(u, v)
            });
        }
    }

    // indices
    uint32_t col = radialSegments + 1;
    for (uint32_t i = 0; i < heightSegments; i++) {
        for (uint32_t j = 0; j < radialSegments; j++) {
            uint32_t index = i * col + j;
            if (ccw) {
                indices.push_back(index);
                indices.push_back(index + 1);
                indices.push_back(index + col);
                indices.push_back(index + 1);
                indices.push_back(index + 1 + col);
                indices.push_back(index + col);
            } else {
                indices.push_back(index);
                indices.push_back(index + col);
                indices.push_back(index + 1);
                indices.push_back(index + 1);
                indices.push_back(index + col);
                indices.push_back(index + 1 + col);
            }
        }
    }

    return GeometryData { vertices, indices };
}

GeometryData Cylinder::Create() const {
    std::vector<GeometryData::Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    size_t vertexCount = (radialSegments + 1) * (heightSegments + 1);
    size_t indexCount = radialSegments * heightSegments * 2 * 3;
    vertices.reserve(vertexCount);
    indices.reserve(indexCount);

    float thetaEnd = thetaStart + thetaLength;

    // vertices
    for (uint32_t i = 0; i <= heightSegments; i++) {
        for (uint32_t j = 0; j <= radialSegments; j++) {
            float u = float(i) / float(heightSegments);
            float v = float(j) / float(radialSegments);
            float r = glm::mix(radiusBottom, radiusTop, u);
            float theta = glm::mix(thetaStart, thetaEnd, v);
            glm::vec3 pos = glm::vec3(
                r * std::cos(theta),
                height * u,
                r * std::sin(theta));
            vertices.push_back(GeometryData::Vertex {
                pos,
                glm::normalize(pos),
                glm::vec2(u, v)
            });
        }
    }

    // indices
    uint32_t col = radialSegments + 1;
    for (uint32_t i = 0; i < heightSegments; i++) {
        for (uint32_t j = 0; j < radialSegments; j++) {
            uint32_t index = i * col + j;
            if (ccw) {
                indices.push_back(index);
                indices.push_back(index + 1);
                indices.push_back(index + col);
                indices.push_back(index + 1);
                indices.push_back(index + 1 + col);
                indices.push_back(index + col);
            } else {
                indices.push_back(index);
                indices.push_back(index + col);
                indices.push_back(index + 1);
                indices.push_back(index + 1);
                indices.push_back(index + col);
                indices.push_back(index + 1 + col);
            }
        }
    }

    return GeometryData { vertices, indices };
}
