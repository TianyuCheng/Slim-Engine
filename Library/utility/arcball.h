#ifndef SLIM_UTILITY_ARCBALL_H
#define SLIM_UTILITY_ARCBALL_H

#include <glm/glm.hpp>

#include "core/vulkan.h"
#include "core/input.h"
#include "utility/camera.h"
#include "utility/transform.h"
#include "utility/interface.h"

namespace slim {

    // arcball control
    class Arcball : public Camera {
    public:
        explicit Arcball(const std::string& name = "arcball");
        virtual ~Arcball() = default;

        void SetExtent(const VkExtent2D &screen);
        void SetDamping(float damping = 0.0f);
        void SetSensitivity(float sensitivity = 1.0f);
        void LookAt(const glm::vec3& eye,
                    const glm::vec3& center,
                    const glm::vec3& updir);

        bool Update(Input* input);

    private:
        bool ProcessRotation(const MouseEvent& mouse);
        bool ProcessScaling(const ScrollEvent& scroll);
        bool ProcessTranslation(const MouseEvent& mouse);

    private:
        VkExtent2D screen;

        // mouse history
        int prevX = 0, prevY = 0;
        int currX = 0, currY = 0;

        // rotation
        glm::vec3 axis;
        float angle = 0.0;

        // movement
        float damping = 0.0;
        float sensitivity = 1.0f;

        // view matrix
        glm::quat rotation;
        glm::vec3 translation;
    };

} // end of SLIM_UTILITY_ARCBALL_H

#endif // SLIM_UTILITY_ARCBALL_H
