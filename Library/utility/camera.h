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

    class SceneNode;

    class Camera {
    public:
        Camera(const std::string &name);
        virtual ~Camera() = default;

        virtual std::string GetName() const { return name; }

        // perform frustum culling
        // return true for things do not need to be culled
        // return false for things needs to be culled
        virtual bool Cull(SceneNode *node, float &distance) const;

        virtual void LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);

        virtual void Ortho(float left, float right, float bottom, float top, float near, float far);

        virtual void Frustum(float left, float right, float bottom, float top, float near, float far);

        virtual void Perspective(float fovy, float aspect, float near, float far);

        void Bind(CommandBuffer* commandBuffer, RenderFrame *renderFrame, PipelineLayout *layout) const;

    private:
        std::string name;
        glm::mat4 view;
        glm::mat4 proj;
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_CAMERA_H
