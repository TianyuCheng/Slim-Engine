#ifndef SLIM_UTILITY_TRANSFORM_H
#define SLIM_UTILITY_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace slim {

    class SceneNode;

    class Transform {
        friend class SceneNode;
    public:
        // identity transform
        explicit Transform() = default;

        // initialize by existing transform matrix
        Transform(const glm::mat4 &xform);

        // initialize by TRS (translation, rotation, scaling)
        explicit Transform(const glm::vec3 &translation,
                           const glm::quat &rotation,
                           const glm::vec3 &scaling = glm::vec3(1.0, 1.0, 1.0));

        virtual ~Transform() = default;

        const glm::mat4& LocalToWorld() const;
        const glm::mat4& WorldToLocal() const;

        void ApplyTransform();
        void ApplyTransform(const Transform &parent);

        void Scale(float x, float y, float z);
        void Rotate(const glm::vec3& axis, float radians);
        void Rotate(float x, float y, float z, float w);
        void Translate(float x, float y, float z);

    private:
        // local transform
        glm::mat4 localXform = glm::mat4(1.0);

        // local/world transform (hierarchical)
        glm::mat4 localToWorld = glm::identity<glm::mat4>();
        glm::mat4 worldToLocal = glm::identity<glm::mat4>();
    };

} // end of namespace slim

#endif // end of SLIM_UTILITY_TRANSFORM_H
