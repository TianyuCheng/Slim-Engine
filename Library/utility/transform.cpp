#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "utility/transform.h"

using namespace slim;

Transform::Transform(const glm::mat4 &xform) {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(xform, localScale, localRotation, localPosition, skew, perspective);
}

void Transform::SetLocalPosition(float tx, float ty, float tz) {
    localPosition.x = tx;
    localPosition.y = ty;
    localPosition.z = tz;
}

void Transform::SetLocalScale(float sx, float sy, float sz) {
    localScale.x = sx;
    localScale.y = sy;
    localScale.z = sz;
}

void Transform::SetLocalRotation(const glm::vec3 &axis, float radians) {
    localRotation = glm::angleAxis(radians, axis);
}

TransformGizmo Transform::AxisGizmo() const {
    const glm::mat3 &mat = glm::mat3_cast(localRotation);
    return TransformGizmo {
        glm::column(mat, 0),
        glm::column(mat, 1),
        glm::column(mat, 2),
    };
}
