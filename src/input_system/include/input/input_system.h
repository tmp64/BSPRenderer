#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H
#include <array>
#include <unordered_map>
#include <string>
#include <SDL.h>
#include <appfw/utils.h>
#include <appfw/appfw.h>
#include <app_base/app_component.h>
#include <input/key_codes.h>

class InputSystem : public AppComponentBase<InputSystem> {
public:
    static constexpr size_t MAX_MOUSE_BUTTONS = 5;

    InputSystem();
    ~InputSystem();

    void tick() override;

    //! @returns whether the key is currently pressed.
    inline bool isKeyPressed(KeyCode key) { return m_pKeyboardState[(int)key]; }
    inline bool isKeyPressed(KeyCode key, int mods) {
        return m_pKeyboardState[(int)key] && m_KeyboardMods == mods;
    }

    //! @returns whether the key has been pressed down this frame.
    inline bool isKeyPressedThisFrame(KeyCode key) { return m_KeyboardStateThisFrame[(int)key]; }
    inline bool isKeyPressedThisFrame(KeyCode key, int mods) {
        return m_KeyboardStateThisFrame[(int)key] && m_KeyboardMods == mods;
    }

    //! Called at the beginning of the tick, before event handling.
    void beginTick();

    //! Handles an SDL2 event.
    //! Called after beginTick, may be called multiple times in one frame
    //! @return true if handled, false if ignored
    bool handleSDLEvent(const SDL_Event &event);

    //! @returns whether the input is grabbed.
    inline bool isInputGrabbed() { return m_bGrabInput; }

    //! Sets whether to grab input.
    //! In grabbed mode:
    //! - mouse is switched to relative mode, cursor is hidden;
    //! - only key up events are passed to ImGui;
    //! - key binds are not triggered.
    void setGrabInput(bool state);

    //! Returns how much mouse has moved since last call to getMouseMovement.
    //! Only valid if input is grabbed.
    //! @param   x   X realtive movement, px
    //! @param   y   Y realtive movement, px
    void getMouseMovement(int &x, int &y);

    //! Returns how much mouse has moved since last call to getMouseMovement.
    //! Doesn't reset it to zero.
    //! Only valid if input is grabbed.
    //! @param   x   X realtive movement, px
    //! @param   y   Y realtive movement, px
    void peekMouseMovement(int &x, int &y);

    //! Discard any pending mouse movement.
    void discardMouseMovement();

private:
    // Keyboard input
    const uint8_t *m_pKeyboardState = nullptr;
    bool m_KeyboardStateThisFrame[SDL_NUM_SCANCODES];
    int m_KeyboardMods = 0;
    int m_bImGuiWantsInput = false;

    // Grabbed input
    bool m_bGrabInput = false;
    int m_iMouseRelX = 0;
    int m_iMouseRelY = 0;

    // Diagnostics UI
    
};

#endif
