#include <algorithm>
#include "utility/boundingbox.h"

using namespace slim;

BoundingBox::BoundingBox(const glm::vec3 &min, const glm::vec3 &max)
    : min(min), max(max) {
    // do nothing
}

BoundingBox BoundingBox::operator+(const BoundingBox &box) {
    return BoundingBox(
        glm::vec3(
            std::min(min.x, box.min.x),
            std::min(min.y, box.min.y),
            std::min(min.z, box.min.z)
        ),
        glm::vec3(
            std::max(max.x, box.max.x),
            std::max(max.y, box.max.y),
            std::max(max.z, box.max.z)
        )
    );
}

BoundingBox& BoundingBox::operator+=(const BoundingBox &box) {
    min.x = std::min(min.x, box.min.x);
    min.y = std::min(min.y, box.min.y);
    min.z = std::min(min.z, box.min.z);
    max.x = std::max(max.x, box.max.x);
    max.y = std::max(max.y, box.max.y);
    max.z = std::max(max.z, box.max.z);
    return *this;
}

BoundingBox& BoundingBox::operator=(const BoundingBox &box) {
    this->min = box.min;
    this->max = box.max;
    return *this;
}

BoundingBox operator*(const glm::mat4& transform, const BoundingBox& box) {
    const glm::vec3& min = box.Min();
    const glm::vec3& max = box.Max();

    glm::vec4 p0 = transform * glm::vec4(min.x, min.y, min.z, 1.0);
    glm::vec4 p1 = transform * glm::vec4(max.x, max.y, max.z, 1.0);
    glm::vec4 p2 = transform * glm::vec4(max.x, min.y, min.z, 1.0);
    glm::vec4 p3 = transform * glm::vec4(min.x, max.y, min.z, 1.0);
    glm::vec4 p4 = transform * glm::vec4(min.x, min.y, max.z, 1.0);
    glm::vec4 p5 = transform * glm::vec4(min.x, min.y, min.z, 1.0);
    glm::vec4 p6 = transform * glm::vec4(min.x, max.y, min.z, 1.0);
    glm::vec4 p7 = transform * glm::vec4(min.x, min.y, max.z, 1.0);

    #define VMAX(C) std::max(std::max(std::max(p0.C, p1.C), std::max(p2.C, p3.C)), std::max(std::max(p4.C, p5.C), std::max(p6.C, p7.C)))
    #define VMIN(C) std::min(std::min(std::min(p0.C, p1.C), std::min(p2.C, p3.C)), std::min(std::min(p4.C, p5.C), std::min(p6.C, p7.C)))
    glm::vec3 nmax = glm::vec3(VMAX(x), VMAX(y), VMAX(z));
    glm::vec3 nmin = glm::vec3(VMIN(x), VMIN(y), VMIN(z));
    #undef VMAX
    #undef VMIN
    return BoundingBox(nmin, nmax);
}
