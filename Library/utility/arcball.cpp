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

Arcball::Arcball() : Camera("arcabll") {
}

Arcball::Arcball(const VkExtent2D &screen) : Camera("arcball"), screen(screen) {
}

void Arcball::Reset() {
    modelAngle = 0.0;
    rotation = glm::mat4(1.0);
    scaling = glm::mat4(1.0);
    translation = glm::mat4(1.0);
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
    const ScrollEvent &scroll = input->Scroll();

    changed |= ProcessRotation(mouse);
    changed |= ProcessScaling(scroll);
    changed |= ProcessTranslation(mouse);

    return changed;
}

bool Arcball::ProcessRotation(const MouseEvent& mouse) {
    int posX = static_cast<int>(mouse.posX);
    int posY = static_cast<int>(mouse.posY);

    if (mouse.button == MouseButton::LeftButton) {
        // on mouse drag start and drag stop
        // (force the prev and curr to be the same to prevent glitches)
        if (mouse.state == MouseState::Pressed) {
            // update mouse positions
            prevX = posX; currX = posX;
            prevY = posY; currY = posY;
        }

        if (mouse.state == MouseState::Released) {
            // update mouse positions
            prevX = posX; currX = posX;
            prevY = posY; currY = posY;
        }

        if (mouse.state == MouseState::Dragging) {
            // update mouse positions
            prevX = currX; currX = posX;
            prevY = currY; currY = posY;
        }
    }

    // update rotation axis
    if (prevX != currX || prevY != currY) {
        glm::vec3 va = GetArcballVector(screen, prevX, prevY);
        glm::vec3 vb = GetArcballVector(screen, currX, currY);
        // compute angle between 2 vectors on the arcball
        float angle = std::acos(std::min(1.0f, glm::dot(va, vb))) * sensitivity;
        // compute axis for rotation
        glm::vec3 axisInCameraCoord = glm::cross(va, vb);
        // compute axis in object coordinate
        modelAngle = angle;
        glm::mat3 worldToLocal = glm::inverse(rotation);
        axisInObjectCoord = glm::mat3(worldToLocal) * axisInCameraCoord;
    }

    bool changed = false;

    // apply rotation inertia for model
    if (std::abs(modelAngle) >= 1e-5) {
        // update model matrix by axis angle
        rotation = glm::rotate(rotation, modelAngle, axisInObjectCoord);
        // apply rotation angle damping
        modelAngle *= damping;
        changed = true;
    }

    return changed;
}

bool Arcball::ProcessScaling(const ScrollEvent& scroll) {
    if (scroll.yOffset == 0) {
        return false;
    }

    // upscaling
    if (scroll.yOffset < 0) {
        const float scale = 1.1;
        scaling = glm::scale(scaling, glm::vec3(scale, scale, scale));
        return true;
    }

    // downscaling
    else {
        const float scale = 0.9;
        scaling = glm::scale(scaling, glm::vec3(scale, scale, scale));
        return true;
    }
}

bool Arcball::ProcessTranslation(const MouseEvent& mouse) {
    int posX = static_cast<int>(mouse.posX);
    int posY = static_cast<int>(mouse.posY);

    bool changed = false;

    // only respond to middle button click
    if (mouse.button == MouseButton::MiddleButton) {
        // on mouse drag start and drag stop
        // (force the prev and curr to be the same to prevent glitches)
        if (mouse.state == MouseState::Pressed) {
            // update mouse positions
            prevX = posX; currX = posX;
            prevY = posY; currY = posY;
            changed = true;
        }

        if (mouse.state == MouseState::Released) {
            // update mouse positions
            prevX = posX; currX = posX;
            prevY = posY; currY = posY;
            changed = true;
        }

        if (mouse.state == MouseState::Dragging) {
            // compute mouse position difference
            int diffX = posX - currX;
            int diffY = posY - currY;
            float diffXScaled = static_cast<float>(+diffX) / (screen.width / 2.0f);
            float diffYScaled = static_cast<float>(-diffY) / (screen.height / 2.0f);
            translation = glm::translate(translation, glm::vec3(diffXScaled, diffYScaled, 0.0));

            // update mouse positions
            prevX = posX; currX = posX;
            prevY = posY; currY = posY;
            changed = true;
        }
    }

    return changed;
}
