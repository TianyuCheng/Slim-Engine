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

        virtual void LookAt(const glm::vec3& eye,
                            const glm::vec3& center,
                            const glm::vec3& up);

        virtual void Ortho(float left, float right,
                           float bottom, float top,
                           float zNear, float zFar);

        virtual void Frustum(float left, float right,
                             float bottom, float top,
                             float zNear, float zFar);

        virtual void Perspective(float fovy, float aspect, float zNear, float zFar);

        virtual void SetViewMatrix(const glm::mat4& view) { this->view = view; }

        virtual void SetProjectionMatrix(const glm::mat4& proj) { this->proj = proj; }

        virtual const glm::mat4& GetView() const { return view; }

        virtual const glm::mat4& GetProjection() const { return proj; }

        virtual const glm::vec3& GetPosition() const { return position; }

        virtual float GetNear() const { return zNear; }

        virtual float GetFar() const { return zFar; }

    private:
        std::string name;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 position;
        float zNear = 0.0f;
        float zFar = 0.0f;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_CAMERA_H
