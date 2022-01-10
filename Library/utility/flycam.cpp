#include <glm/gtx/string_cast.hpp>
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

    glm::vec3 right;

    // record camera info
    pos = eye;
    look = glm::normalize(center - eye);
    updir = glm::normalize(up);
    right = glm::normalize(glm::cross(look, updir));
    updir = glm::normalize(glm::cross(right, look));
    // std::cout << "look: " << glm::to_string(look) << std::endl;
    // std::cout << "right: " << glm::to_string(right) << std::endl;
    // std::cout << "updir: " << glm::to_string(updir) << std::endl;

    // infer angleH and angleV
    angleV = std::asin(look.y);
    angleH = std::asin(look.x / std::cos(angleV));
    float dx = std::cos(angleV) * std::sin(angleH);
    float dy = std::sin(angleV);
    float dz = std::cos(angleV) * std::cos(angleH);
    float rx = std::sin(angleH - M_PI_2);
    float ry = 0.0;
    float rz = std::cos(angleH - M_PI_2);

    // in case we are not entirely correct, do a simple check and update inferred angles
    if (std::abs(rx - right.x) > 1e-4 || std::abs(rz - right.z) > 1e-4) {
        angleH -= M_PI_2;
        dx = std::cos(angleV) * std::sin(angleH);
        dy = std::sin(angleV);
        dz = std::cos(angleV) * std::cos(angleH);
        rx = std::sin(angleH - M_PI_2);
        ry = 0.0;
        rz = std::cos(angleH - M_PI_2);
    }

    // look = glm::vec3(dx, dy, dz);
    // right = glm::vec3(rx, ry, rz);
    // updir = glm::cross(right, look);
    // std::cout << "look: " << glm::to_string(look) << std::endl;
    // std::cout << "right: " << glm::to_string(right) << std::endl;
    // std::cout << "updir: " << glm::to_string(updir) << std::endl;
    // Camera::LookAt(pos, pos + look, updir);
}

void Flycam::Update(Input *input, Time *time) {
    assert(screen.width != 0 && screen.height != 0);

    const MouseEvent &mouse = input->Mouse();
    bool updated = false;
    float deltaTime = time->Delta();

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

