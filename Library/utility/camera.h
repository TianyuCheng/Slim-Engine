#ifndef SLIM_UTILITY_CAMERA_H
#define SLIM_UTILITY_CAMERA_H

#include <string>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "core/pipeline.h"
#include "core/commands.h"
#include "core/renderframe.h"
#include "utility/interface.h"

namespace slim {

    class Camera : public ReferenceCountable {
    public:
        Camera(const std::string &name);
        virtual ~Camera() = default;

        virtual std::string GetName() const { return name; }

        virtual void LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);

        virtual void Ortho(float left, float right, float bottom, float top, float near, float far);

        virtual void Frustum(float left, float right, float bottom, float top, float near, float far);

        virtual void Perspective(float fovy, float aspect, float near, float far);

        void SetViewMatrix(const glm::mat4& view) { this->view = view; }

        void SetProjectionMatrix(const glm::mat4& proj) { this->proj = proj; }

        const glm::mat4& GetView() const { return view; }

        const glm::mat4& GetProjection() const { return proj; }

    private:
        std::string name;
        glm::mat4 view;
        glm::mat4 proj;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_CAMERA_H
