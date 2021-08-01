#ifndef SLIM_UTILITY_BOUNDING_BOX_H
#define SLIM_UTILITY_BOUNDING_BOX_H

#include <limits>
#include <glm/glm.hpp>

namespace slim {

    constexpr float INF = std::numeric_limits<float>::infinity();

    class BoundingBox final {
    public:
        explicit BoundingBox() = default;
        explicit BoundingBox(const glm::vec3 &min, const glm::vec3 &max);
        virtual ~BoundingBox() = default;

        BoundingBox operator+(const BoundingBox &box);
        BoundingBox& operator+=(const BoundingBox &box);
    private:
        glm::vec3 min = glm::vec3(+INF, +INF, +INF);
        glm::vec3 max = glm::vec3(-INF, -INF, -INF);
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_BOUNDING_BOX_H
