#include <cassert>
#include "utility/flycam.h"

using namespace slim;

Flycam::Flycam(const std::string& name) : Camera(name) {
}

Flycam::~Flycam() {
}

void Flycam::SetWalkSpeed(float speed) {
    walkSpeed = speed;
}

void Flycam::SetRotateSpeed(float speed) {
    rotateSpeed = speed;
}

void Flycam::SetExtent(const VkExtent2D &extent) {
    screen = extent;
}

void Flycam::LookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) {
    Camera::LookAt(eye, center, up);
    pos = eye;
    look = glm::normalize(center - eye);
    updir = glm::normalize(up);
}

void Flycam::Update(Input *input, const Time &time) {
    assert(screen.width != 0 && screen.height != 0);

    const MouseEvent &mouse = input->Mouse();
    bool updated = false;
    float deltaTime = time.Delta();

    if (mouse.state == MouseState::Dragging) {
        angleH += float(mouse.moveX) / (2 * screen.width);
        angleV += float(mouse.moveY) / (2 * screen.height);
        updated = true;
    }

    if (input->IsKeyPressed(KeyCode::Up) || input->IsKeyPressed(KeyCode::KeyW)) {
        pos += look * walkSpeed * deltaTime;
        updated = true;
    }
    if (input->IsKeyPressed(KeyCode::Down) || input->IsKeyPressed(KeyCode::KeyS)) {
        pos -= look * walkSpeed * deltaTime;
        updated = true;
    }
    if (input->IsKeyPressed(KeyCode::Left) || input->IsKeyPressed(KeyCode::KeyA)) {
        angleH += rotateSpeed * deltaTime;
        updated = true;
    }
    if (input->IsKeyPressed(KeyCode::Right) || input->IsKeyPressed(KeyCode::KeyD)) {
        angleH -= rotateSpeed * deltaTime;
        updated = true;
    }

    if (updated) {
        look = glm::vec3(
            std::cos(angleV) * std::sin(angleH),
            std::sin(angleV),
            std::cos(angleV) * std::cos(angleH)
        );
        glm::vec3 right = glm::vec3(
            std::sin(angleH - M_PI_2),
            0,
            std::cos(angleH - M_PI_2)
        );
        updir = glm::cross(right, look);
        Camera::LookAt(pos, pos + look, updir);
    }
}

