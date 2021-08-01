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

// bool Camera::Cull(SceneNode *, float &) const {
//     return true;
// }

void Camera::Bind(CommandBuffer* commandBuffer, RenderFrame *renderFrame, PipelineLayout *layout) const {
    struct Uniform {
        glm::mat4 view;
        glm::mat4 proj;
    } uniform;

    uniform.view = view;
    uniform.proj = proj;

    auto uniformBuffer = renderFrame->RequestUniformBuffer(uniform);
    uniformBuffer->Flush();

    auto descriptor = SlimPtr<Descriptor>(renderFrame->GetDescriptorPool(), layout);
    descriptor->SetUniform("Camera", uniformBuffer);
    commandBuffer->BindDescriptor(descriptor);
}
