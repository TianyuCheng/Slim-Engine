#include <glm/gtc/matrix_transform.hpp>
#include "utility/camera.h"

using namespace slim;

Camera::Camera(const std::string &name) : name(name) {
}

void Camera::Ortho(float left, float right, float bottom, float top, float near, float far) {
    proj = glm::ortho(left, right, bottom, top, near, far);
}

void Camera::Frustum(float left, float right, float bottom, float top, float near, float far) {
    proj = glm::frustum(left, right, bottom, top, near, far);
}

void Camera::Perspective(float fovy, float aspect, float near, float far) {
    proj = glm::perspective(fovy, aspect, near, far);
}

void Camera::LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) {
    view = glm::lookAt(eye, center, up);
}

void Camera::Cull(SceneNode *node) const {
}
