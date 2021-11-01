#ifndef INPUT_KEY_BIND_H
#define INPUT_KEY_BIND_H
#include <appfw/utils.h>
#include <input/key_codes.h>
#include <input/input_system.h>

class KeyBind : appfw::NoMove {
public:
    //! Constructs a key bind.
    //! @param  title       Title of the key bind (will be shown in GUI)
    //! @param  defaultKey  Default key
    //! @param  defaultMods Default key modifiers (KMOD_*)
    KeyBind(const char *title, KeyCode defaultKey, int defaultMods = KMOD_NONE);

    //! @returns whether the key is currently pressed down
    inline bool isHeldDown() { return InputSystem::get().isKeyPressed(m_KeyCode, m_ModKeys); }

    //! @returns whether the key is pressed this frame.
    inline bool isPressed() {
        return InputSystem::get().isKeyPressedThisFrame(m_KeyCode, m_ModKeys);
    }

    //! @returns the shortcut string for ImGui
    inline const char *getShortcut() { return m_szShortcut; }

    inline const char *getTitle() { return m_pszTitle; }
    inline KeyCode getDefaultKeyCode() { return m_DefaultKey; }
    inline int getDefaultModKeys() { return m_DefaultMods; }

    inline KeyCode getKeyCode() { return m_KeyCode; }
    inline int getModKeys() { return m_ModKeys; }

    void setKeyCode(KeyCode key, int mods);

    //! @returns the list of all key binds.
    static std::list<KeyBind *> &getList();

private:
    const char *m_pszTitle = nullptr;
    KeyCode m_DefaultKey = KeyCode::None;
    int m_DefaultMods = KMOD_NONE;

    KeyCode m_KeyCode = KeyCode::None;
    int m_ModKeys = KMOD_NONE;
    char m_szShortcut[32];
};

#endif
