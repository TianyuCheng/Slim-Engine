#ifndef SLIM_UTILITY_BOUNDING_BOX_H
#define SLIM_UTILITY_BOUNDING_BOX_H

#include <glm/glm.hpp>

namespace slim {

    class BoundingBox final {
    public:
        explicit BoundingBox() = default;
        explicit BoundingBox(const glm::vec3 &min, const glm::vec3 &max);
        virtual ~BoundingBox() = default;
        BoundingBox& Merge(const BoundingBox &box);
    private:
        glm::vec3 min = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 max = glm::vec3(0.0, 0.0, 0.0);
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_BOUNDING_BOX_H
