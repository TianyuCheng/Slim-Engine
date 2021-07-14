#ifndef SLIM_UTILITY_CAMERA_H
#define SLIM_UTILITY_CAMERA_H

#include <glm/glm.hpp>

namespace slim {

    class SceneNode;

    class Camera {
    public:
        // perform frustum culling
        // * true for cullable objects -> totally out of camera view
        // * false for non-cullable objects
        virtual bool Cull(const SceneNode *node) = 0;
        virtual void LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up);
        virtual glm::mat4 GetViewProjection() const { return proj * view; }
    protected:
        glm::mat4 view;
        glm::mat4 proj;
    };

    class OrthographicCamera : public Camera {
    public:
        OrthographicCamera();

        // * true for cullable objects -> totally out of camera view
        // * false for non-cullable objects
        virtual bool Cull(const SceneNode *) { return false; }
    };

    class PerspectiveCamera : public Camera {
    public:
        PerspectiveCamera();

        // * true for cullable objects -> totally out of camera view
        // * false for non-cullable objects
        virtual bool Cull(const SceneNode *) { return false; }
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_CAMERA_H
