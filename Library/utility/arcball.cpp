#include "utility/arcball.h"
#include <iostream>

using namespace slim;

glm::vec3 GetArcballVector(const VkExtent2D &screen, int x, int y) {
    // (x, y) is the coordinate on the screen

    // translate screen-space coordinate into [-1, 1]
    glm::vec3 P = glm::vec3(
        static_cast<float>(x) / (screen.width * 2.0),
        static_cast<float>(y) / (screen.height * 2.0),
        1.0
    );

    P.y = -P.y;

    float squared = P.x * P.x + P.y * P.y;
    if (squared <= 1.0)
        P.z = std::sqrt(1.0 - squared); // Pythagoras
    else
        P = glm::normalize(P);

    return P;
}

Arcball::Arcball(const VkExtent2D &screen) : screen(screen) {
}

void Arcball::Reset() {
    angle = 0.0;
    xform = glm::mat4(1.0);
}

void Arcball::SetDamping(float value) {
    damping = value;
}

void Arcball::SetSensitivity(float value) {
    sensitivity = value;
}

void Arcball::SetExtent(const VkExtent2D &extent) {
    screen = extent;
}

bool Arcball::Update(Input* input) {
    assert(screen.width != 0 && screen.height != 0);
    bool changed = false;

    const MouseEvent &mouse = input->Mouse();

    float posX = mouse.posX;
    float posY = mouse.posY;

    // on mouse drag start and drag stop
    // (force the prev and curr to be the same to prevent glitches)
    if (mouse.state == MouseState::Pressed || mouse.state == MouseState::Released) {
        // update mouse positions
        prevX = posX; currX = posX;
        prevY = posY; currY = posY;
        changed = true;
    }
    else if (mouse.state == MouseState::Dragging) {
        // update mouse positions
        prevX = currX; currX = posX;
        prevY = currY; currY = posY;
        changed = true;
    }

    // update scaling
    const ScrollEvent &scroll = input->Scroll();
    if (scroll.yOffset != 0) {
        if (scroll.yOffset < 0) {
            const float scale = 1.1;
            xform.Scale(scale, scale, scale);
            changed = true;
        } else {
            const float scale = 0.9;
            xform.Scale(scale, scale, scale);
            changed = true;
        }
    }

    // rotate inertia
    if (prevX != currX || prevY != currY) {
        glm::vec3 va = GetArcballVector(screen, prevX, prevY);
        glm::vec3 vb = GetArcballVector(screen, currX, currY);
        // compute angle between 2 vectors on the arcball
        angle = std::acos(std::min(1.0f, glm::dot(va, vb))) * sensitivity;
        // compute axis for rotation
        glm::vec3 axisInCameraCoord = glm::cross(va, vb);
        // compute axis in object coordinate
        xform.ApplyTransform();
        axisInObjectCoord = glm::mat3(xform.WorldToLocal()) * axisInCameraCoord;
        changed = true;
    }

    // rotate inertia
    if (angle != 0) {
        // update model matrix by axis angle
        xform.Rotate(axisInObjectCoord, angle);
        angle *= damping;
        changed = true;
    }
    return changed;
}
