#ifndef SLIM_UTILITY_ARCBALL_H
#define SLIM_UTILITY_ARCBALL_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "core/input.h"
#include "utility/camera.h"
#include "utility/transform.h"
#include "utility/interface.h"

namespace slim {

    // arcball control
    class Arcball : public Camera {
    public:
        explicit Arcball();
        explicit Arcball(const VkExtent2D &screen);
        virtual ~Arcball() = default;

        void Reset();

        void SetDamping(float damping = 0.0f);
        void SetSensitivity(float sensitivity = 1.0f);
        void SetExtent(const VkExtent2D &screen);
        bool Update(Input* input);

        const glm::mat4 GetModelMatrix(bool applyScaling = true) const {
            return applyScaling ? (translation * scaling * rotation)
                                : (translation * rotation);
        }

    private:
        bool ProcessRotation(const MouseEvent& mouse);
        bool ProcessScaling(const ScrollEvent& scroll);
        bool ProcessTranslation(const MouseEvent& mouse);

    private:
        VkExtent2D screen;
        int prevX = 0, prevY = 0;
        int currX = 0, currY = 0;
        float modelAngle = 0.0;
        float damping = 0.0;
        float sensitivity = 1.0f;
        glm::vec3 axisInObjectCoord;

        // model matrices
        glm::mat4 rotation = glm::mat4(1.0);
        glm::mat4 scaling = glm::mat4(1.0);
        glm::mat4 translation = glm::mat4(1.0);
    };

} // end of SLIM_UTILITY_ARCBALL_H

#endif // SLIM_UTILITY_ARCBALL_H
