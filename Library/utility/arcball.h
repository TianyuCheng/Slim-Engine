#ifndef SLIM_UTILITY_ARCBALL_H
#define SLIM_UTILITY_ARCBALL_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "core/input.h"
#include "utility/transform.h"
#include "utility/interface.h"

namespace slim {

    // arcball control
    class Arcball : public ReferenceCountable {
    public:
        explicit Arcball() = default;
        explicit Arcball(const VkExtent2D &screen);
        virtual ~Arcball() = default;

        void Reset();

        void SetDamping(float damping = 0.0f);
        void SetSensitivity(float sensitivity = 1.0f);
        void SetExtent(const VkExtent2D &screen);
        bool Update(Input* input);

        const Transform& GetTransform() const { return xform; }
        const Transform& GetTransformNoScale() const { return xformNoScale; }

    private:
        VkExtent2D screen;
        int prevX = 0, prevY = 0;
        int currX = 0, currY = 0;
        float angle = 0.0;
        float damping = 0.0;
        float sensitivity = 1.0f;
        glm::vec3 axisInObjectCoord;
        Transform xform;
        Transform xformNoScale;
    };

} // end of SLIM_UTILITY_ARCBALL_H

#endif // SLIM_UTILITY_ARCBALL_H
