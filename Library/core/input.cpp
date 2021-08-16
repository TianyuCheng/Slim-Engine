#include "imgui.h"
#include "core/input.h"

using namespace slim;

namespace {

    static bool ImGuiCapturedMouse() {
        ImGuiContext* context = ImGui::GetCurrentContext();
        if (context) {
            ImGuiIO& io = ImGui::GetIO();
            return io.WantCaptureMouse;
        }
        return false;
    }

    static bool ImGuiCapturedTextInput() {
        ImGuiContext* context = ImGui::GetCurrentContext();
        if (context) {
            ImGuiIO& io = ImGui::GetIO();
            return io.WantTextInput;
        }
        return false;
    }

    static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
        // it does not matter for the case of hover
        // no need to check if mouse is captured by imgui

        // pass the event to cursor position callback
        Input *input = reinterpret_cast<Input*>(glfwGetWindowUserPointer(window));
        input->CursorPositionCallback(xpos, ypos);
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        (void) mods;

        // if imgui has the focus, then we do not process mouse button
        if (ImGuiCapturedMouse()) return;

        // pass the event to cusor button callback
        Input *input = reinterpret_cast<Input*>(glfwGetWindowUserPointer(window));

        // state
        MouseState state;
        switch (action) {
            case GLFW_PRESS:   state = MouseState::Pressed;  break;
            case GLFW_RELEASE: state = MouseState::Released; break;
            default:           state = MouseState::None;     break;
        }

        // button
        MouseButton mouse;
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:   mouse = MouseButton::LeftButton;   break;
            case GLFW_MOUSE_BUTTON_RIGHT:  mouse = MouseButton::RightButton;  break;
            case GLFW_MOUSE_BUTTON_MIDDLE: mouse = MouseButton::MiddleButton; break;
            default:                       mouse = MouseButton::None;         break;
        }

        input->MouseButtonCallback(mouse, state);
    }

    static void KeyCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods) {
        (void) scancode;

        // if imgui has the focus, then we do not process key event
        if (ImGuiCapturedTextInput()) return;

        Input *input = reinterpret_cast<Input*>(glfwGetWindowUserPointer(window));
        // key code
        KeyCode key = static_cast<KeyCode>(keycode);
        // key state
        KeyState state = KeyState::None;
        if (action == GLFW_PRESS) state = KeyState::Pressed;
        if (action == GLFW_RELEASE) state = KeyState::Released;
        // key modifiers
        KeyModifier modifiers = static_cast<KeyModifier>(mods);
        // redirect callback
        input->KeyCallback(key, state, modifiers);
    }

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        // if imgui has the focus, then we do not process mouse button
        if (ImGuiCapturedMouse()) return;

        // pass the event to cusor button callback
        Input *input = reinterpret_cast<Input*>(glfwGetWindowUserPointer(window));
        input->ScrollCallback(xoffset, yoffset);
    }
};

Input::Input(Window* window) : window(*window) {
    glfwSetWindowUserPointer(this->window,   this);
    glfwSetCursorPosCallback(this->window,   ::CursorPositionCallback);
    glfwSetMouseButtonCallback(this->window, ::MouseButtonCallback);
    glfwSetKeyCallback(this->window,         ::KeyCallback);
    glfwSetScrollCallback(this->window,      ::ScrollCallback);
}

void Input::Reset() {
    // update previous mouse event, and reset current mouse event
    prevMouseEvent = currMouseEvent;
    currMouseEvent.state = MouseState::None;
    currMouseEvent.moveX = 0.0f;
    currMouseEvent.moveY = 0.0f;

    // reset key event
    keyEvent.key = KeyCode::None;
    keyEvent.mods = KeyModifier::None;
    keyEvent.state = KeyState::None;

    // reset scroll event
    scrollEvent.xOffset = 0.0f;
    scrollEvent.yOffset = 0.0f;
}

void Input::CursorPositionCallback(double xpos, double ypos) {
    currMouseEvent.posX = xpos;
    currMouseEvent.posY = ypos;
    currMouseEvent.moveX = xpos - prevMouseEvent.posX;
    currMouseEvent.moveY = ypos - prevMouseEvent.posY;
    // NOTE: augment existing mouse button state
    if (currMouseEvent.state == MouseState::None) {
        if (currMouseEvent.posX != prevMouseEvent.posX ||
            currMouseEvent.posY != prevMouseEvent.posY) {
            if (mouseClicked) currMouseEvent.state = MouseState::Dragging;
            else              currMouseEvent.state = MouseState::Moving;
        }
    }
}

void Input::MouseButtonCallback(MouseButton button, MouseState state) {
    currMouseEvent.state = state;
    currMouseEvent.button = button;
    if (state == MouseState::Pressed)  {
        mouseClicked = true;
    }
    if (state == MouseState::Released) {
        mouseClicked = false;
    }
}

void Input::KeyCallback(KeyCode key, KeyState state, KeyModifier mods) {
    keyEvent.key   = key;
    keyEvent.mods  = mods;
    keyEvent.state = state;
}

void Input::ScrollCallback(double xOffset, double yOffset) {
    scrollEvent.xOffset = xOffset;
    scrollEvent.yOffset = yOffset;
}

bool Input::IsKeyPressed(KeyCode key) const {
    return glfwGetKey(window, static_cast<int>(key)) == GLFW_PRESS;
}

bool Input::IsKeyReleased(KeyCode key) const {
    return glfwGetKey(window, static_cast<int>(key)) == GLFW_RELEASE;
}
