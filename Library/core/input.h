#ifndef SLIM_CORE_INPUT_H
#define SLIM_CORE_INPUT_H

#include "core/window.h"
#include "utility/interface.h"

namespace slim {

    enum class KeyCode {
        None         = -1,

        KeyA         = GLFW_KEY_A,
        KeyB         = GLFW_KEY_B,
        KeyC         = GLFW_KEY_C,
        KeyD         = GLFW_KEY_D,
        KeyE         = GLFW_KEY_E,
        KeyF         = GLFW_KEY_F,
        KeyG         = GLFW_KEY_G,
        KeyH         = GLFW_KEY_H,
        KeyI         = GLFW_KEY_I,
        KeyJ         = GLFW_KEY_J,
        KeyK         = GLFW_KEY_K,
        KeyL         = GLFW_KEY_L,
        KeyM         = GLFW_KEY_M,
        KeyN         = GLFW_KEY_N,
        KeyO         = GLFW_KEY_O,
        KeyP         = GLFW_KEY_P,
        KeyQ         = GLFW_KEY_Q,
        KeyR         = GLFW_KEY_R,
        KeyS         = GLFW_KEY_S,
        KeyT         = GLFW_KEY_T,
        KeyU         = GLFW_KEY_U,
        KeyV         = GLFW_KEY_V,
        KeyW         = GLFW_KEY_W,
        KeyX         = GLFW_KEY_X,
        KeyY         = GLFW_KEY_Y,
        KeyZ         = GLFW_KEY_Z,

        Key1         = GLFW_KEY_1,
        Key2         = GLFW_KEY_2,
        Key3         = GLFW_KEY_3,
        Key4         = GLFW_KEY_4,
        Key5         = GLFW_KEY_5,
        Key6         = GLFW_KEY_6,
        Key7         = GLFW_KEY_7,
        Key8         = GLFW_KEY_8,
        Key9         = GLFW_KEY_9,

        Enter        = GLFW_KEY_ENTER,
        Space        = GLFW_KEY_SPACE,
        Comma        = GLFW_KEY_COMMA,
        Period       = GLFW_KEY_PERIOD,
        Backspace    = GLFW_KEY_BACKSPACE,
        Semicolon    = GLFW_KEY_SEMICOLON,
        Equal        = GLFW_KEY_EQUAL,
        Slash        = GLFW_KEY_SLASH,
        Apostrophe   = GLFW_KEY_APOSTROPHE,
        Delete       = GLFW_KEY_DELETE,
        Insert       = GLFW_KEY_INSERT,
        Tab          = GLFW_KEY_TAB,
        CapsLock     = GLFW_KEY_CAPS_LOCK,
        Home         = GLFW_KEY_HOME,
        Minus        = GLFW_KEY_MINUS,

        Up           = GLFW_KEY_UP,
        Down         = GLFW_KEY_DOWN,
        Left         = GLFW_KEY_LEFT,
        Right        = GLFW_KEY_RIGHT,

        PageUp       = GLFW_KEY_PAGE_UP,
        PageDown     = GLFW_KEY_PAGE_DOWN,
    };

    enum class KeyModifier {
        None         = 0,
        Shift        = GLFW_MOD_SHIFT,
        Control      = GLFW_MOD_CONTROL,
        Alt          = GLFW_MOD_ALT,
        Super        = GLFW_MOD_SUPER,
    };

    enum class KeyState {
        None         = 0x0,
        Pressed      = 0x1,
        Released     = 0x2,
    };

    enum class MouseButton {
        None         = 0x0,
        LeftButton   = 0x1,
        MiddleButton = 0x2,
        RightButton  = 0x4,
    };

    enum class MouseState {
        None         = 0x0,
        Moving       = 0x1,
        Pressed      = 0x2,
        Dragging     = 0x3,
        Released     = 0x4,
    };

    struct ScrollEvent {
        float xOffset;
        float yOffset;
    };

    struct MouseEvent {
        MouseState  state;
        MouseButton button;
        float       posX;
        float       posY;
        float       moveX;
        float       moveY;
    };

    struct KeyEvent {
        KeyState    state;
        KeyCode     key;
        KeyModifier mods;
        bool HasModifier(KeyModifier mod) const {
            return static_cast<uint32_t>(mods) & static_cast<uint32_t>(mod);
        }
    };

    class Input : public ReferenceCountable {
    public:
        explicit Input(Window* window);
        virtual ~Input() = default;

        void Reset();

        // used for continuous key press
        bool IsKeyPressed (KeyCode key) const;
        bool IsKeyReleased(KeyCode key) const;

        // get mouse and key events
        MouseEvent  Mouse()  const { return currMouseEvent; }
        KeyEvent    Key()    const { return keyEvent;       }
        ScrollEvent Scroll() const { return scrollEvent;    }

        void CursorPositionCallback(double xpos, double ypos);
        void MouseButtonCallback(MouseButton button, MouseState state);
        void KeyCallback(KeyCode key, KeyState state, KeyModifier mods);
        void ScrollCallback(double xOffset, double yOffset);

    private:
        GLFWwindow *window = nullptr;
        KeyEvent    keyEvent;
        MouseEvent  prevMouseEvent;
        MouseEvent  currMouseEvent;
        ScrollEvent scrollEvent;
        bool mouseClicked = false;
    };

} // end of SLIM_CORE_INPUT_H

#endif // SLIM_CORE_INPUT_H
