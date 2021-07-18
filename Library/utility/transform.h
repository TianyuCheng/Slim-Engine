#ifndef SLIM_UTILITY_TRANSFORM_H
#define SLIM_UTILITY_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace slim::transform {

    // identity
    glm::mat4 Identity();

    // scaling
    glm::mat4 Scale(float sx, float sy, float sz);
    glm::mat4 Scale(float s);
    glm::mat4 ScaleX(float s);
    glm::mat4 ScaleY(float s);
    glm::mat4 ScaleZ(float s);

    // translation
    glm::mat4 Translate(float tx, float ty, float tz);
    glm::mat4 TranslateX(float t);
    glm::mat4 TranslateY(float t);
    glm::mat4 TranslateZ(float t);

    // rotation
    glm::mat4 Rotate(const glm::vec3 &axis, float radians);

} // end of namespace slim::transform

namespace slim {

    struct TransformGizmo {
        glm::vec3 right;   // red axis in world space
        glm::vec3 up;      // blue axis in world space
        glm::vec3 forward; // green axis in the world space
    };

    class Transform {
    public:
        explicit Transform() = default;
        explicit Transform(const glm::mat4 &xform);
        virtual ~Transform() = default;

        void SetLocalPosition(float tx, float ty, float tz);
        void SetLocalScale(float sx, float sy, float sz);
        void SetLocalRotation(const glm::vec3 &axis, float radians);

        TransformGizmo AxisGizmo() const;

    private:
        // local transform
        glm::vec3 localPosition = glm::vec3();
        glm::quat localRotation = glm::quat();
        glm::vec3 localScale    = glm::vec3(1.0f, 1.0f, 1.0f);

        // local/world transform
        glm::mat4 localToWorld = glm::identity<glm::mat4>();
        glm::mat4 worldToLocal = glm::identity<glm::mat4>();
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_TRANSFORM_H
