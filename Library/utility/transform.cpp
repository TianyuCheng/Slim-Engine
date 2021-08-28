#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "utility/transform.h"

using namespace slim;

Transform::Transform(const glm::mat4 &xform) : localXform(xform) {
    // compute localToWorld and worldToLocal
    localToWorld = localXform;
    worldToLocal = glm::inverse(localToWorld);
}

Transform::Transform(const glm::vec3 &translation,
                     const glm::quat &rotation,
                     const glm::vec3 &scaling) {
    localXform = glm::mat4_cast(rotation);
    localXform = glm::scale(localXform, scaling);
    localXform = glm::translate(localXform, translation);

    // compute localToWorld and worldToLocal
    localToWorld = localXform;
    worldToLocal = glm::inverse(localToWorld);
}

void Transform::ApplyTransform() {
    localToWorld = localXform;
    worldToLocal = glm::inverse(localToWorld);
}

void Transform::ApplyTransform(const Transform &parent) {
    localToWorld = parent.localToWorld * localXform;
    worldToLocal = worldToLocal * parent.worldToLocal;
}

const glm::mat4& Transform::LocalToWorld() const {
    return localToWorld;
}

const glm::mat4& Transform::WorldToLocal() const {
    return worldToLocal;
}

void Transform::Scale(float x, float y, float z) {
    localXform = glm::scale(localXform, glm::vec3(x, y, z));
}

void Transform::Rotate(const glm::vec3& axis, float radians) {
    localXform = glm::rotate(localXform, radians, axis);
}

void Transform::Rotate(float x, float y, float z, float w) {
    localXform = glm::mat4_cast(glm::quat(x, y, z, w)) * localXform;
}

void Transform::Translate(float x, float y, float z) {
    localXform = glm::translate(localXform, glm::vec3(x, y, z));
}
