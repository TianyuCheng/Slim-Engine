#include <glm/gtc/matrix_transform.hpp>
#include "utility/camera.h"

using namespace slim;

Camera::Camera(const std::string &name) : name(name) {
}

void Camera::Ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    proj = glm::ortho(left, right, bottom, top, zNear, zFar);
    this->zNear = zNear;
    this->zFar = zFar;
}

void Camera::Frustum(float left, float right, float bottom, float top, float zNear, float zFar) {
    proj = glm::frustum(left, right, bottom, top, zNear, zFar);
    this->zNear = zNear;
    this->zFar = zFar;
}

void Camera::Perspective(float fovy, float aspect, float zNear, float zFar) {
    proj = glm::perspective(fovy, aspect, zNear, zFar);
    this->zNear = zNear;
    this->zFar = zFar;
}

void Camera::LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) {
    view = glm::lookAt(eye, center, up);
    position = eye;
}
