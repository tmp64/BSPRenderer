#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H
#include <array>
#include <unordered_map>
#include <string>
#include <SDL.h>
#include <appfw/utils.h>

class InputSystem : appfw::NoCopy {
public:
    static constexpr size_t MAX_MOUSE_BUTTONS = 5;

    static inline InputSystem &get() { return *m_sInstance; }

    InputSystem();
    ~InputSystem();

    /**
     * Handles an SDL2 event.
     * @return true if handled, false if ignored
     */
    bool handleSDLEvent(SDL_Event event);

    /**
     * Called every tick to perform actions for held keys.
     */
    void tick();

    /**
     * If true, keys and mouse movements are sent to the app, cursor is hidden,
     * If false, keys and mouse are sent to GUI.
     */
    bool isInputGrabbed();

    /**
     * Enables/disables input grab.
     */
    void setGrabInput(bool state);

    /**
     * Returns SDL scancode for a key name or SDL_SCANCODE_UNKNOWN.
     */
    SDL_Scancode getScancodeForKey(const std::string &key);

    /**
     * Returns command bound to the key.
     */
    const appfw::ParsedCommand &getKeyBind(SDL_Scancode key);

    /**
     * Binds a command to a key.
     */
    void bindKey(SDL_Scancode key, const std::string &cmd);

    /**
     * Binds a command to a key.
     */
    void bindKey(SDL_Scancode key, const appfw::ParsedCommand &cmd);

    /**
     * Returns how much mouse has moved since last call to getMouseMovement.
     * Only valid if input is grabbed.
     * @param   x   X realtive movement, px
     * @param   y   Y realtive movement, px
     */
    void getMouseMovement(int &x, int &y);

    /**
     * Returns how much mouse has moved since last call to getMouseMovement.
     * Doesn't reset it to zero.
     * Only valid if input is grabbed.
     * @param   x   X realtive movement, px
     * @param   y   Y realtive movement, px
     */
    void peekMouseMovement(int &x, int &y);

    /**
     * Discard any pending mouse movement.
     */
    void discardMouseMovement();

private:
    static inline InputSystem *m_sInstance = nullptr;

    bool m_bGrabInput = false;
    int m_iMouseRelX = 0;
    int m_iMouseRelY = 0;
    std::unordered_map<std::string, SDL_Scancode> m_ScancodeMap;
    std::array<appfw::ParsedCommand, SDL_NUM_SCANCODES> m_KeyBinds;
    std::array<appfw::ParsedCommand, MAX_MOUSE_BUTTONS + 1> m_MouseBinds;

    void createScancodeMap();
};

#endif
