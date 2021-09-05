#ifndef SLIM_UTILITY_FLYCAM_H
#define SLIM_UTILITY_FLYCAM_H

#include <cmath>
#include <string>

#include "core/input.h"
#include "utility/time.h"
#include "utility/camera.h"

namespace slim {

    // flycam control
    class Flycam : public Camera {
    public:
        explicit Flycam(const std::string &name = "flycam");
        virtual ~Flycam();

        void SetExtent(const VkExtent2D &screen);
        void SetWalkSpeed(float speed);
        void SetRotateSpeed(float speed);

        void Update(Input *input, const Time &time);

        // camera view matrix
        virtual void LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) override;

    private:
        glm::vec3 pos = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 look = glm::vec3(0.0, 0.0, 1.0);
        glm::vec3 updir = glm::vec3(0.0, 1.0, 0.0);

        float angleH = 0.0f;
        float angleV = 0.0f;

        VkExtent2D screen;
        float walkSpeed = 10.0;
        float rotateSpeed = M_PI_2;
    };

}; // end of slim namespace

#endif // SLIM_ENGINE_UTILITY_H
