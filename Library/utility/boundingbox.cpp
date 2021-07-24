#include <algorithm>
#include "utility/boundingbox.h"

using namespace slim;

BoundingBox::BoundingBox(const glm::vec3 &min, const glm::vec3 &max)
    : min(min), max(max) {
    // do nothing
}

BoundingBox& BoundingBox::Merge(const BoundingBox &box) {
    min.x = std::min(min.x, box.min.x);
    min.y = std::min(min.y, box.min.y);
    min.z = std::min(min.z, box.min.z);
    max.x = std::max(max.x, box.max.x);
    max.y = std::max(max.y, box.max.y);
    max.z = std::max(max.z, box.max.z);
    return *this;
}
