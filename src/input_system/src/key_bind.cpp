#include <input/key_bind.h>

KeyBind::KeyBind(const char *title, KeyCode defaultKey, int defaultMods) {
    m_pszTitle = title;
    m_DefaultKey = defaultKey;
    m_DefaultMods = defaultMods & SUPPORTED_KMOD;

    getList().push_back(this);
}

void KeyBind::setKeyCode(KeyCode key, int mods) {
    m_KeyCode = key;
    m_ModKeys = mods;

    std::string str;

    if (mods & KMOD_LCTRL) {
        str += "CTRL+";
    }

    if (mods & KMOD_LALT) {
        str += "ALT+";
    }

    if (mods & KMOD_LSHIFT) {
        str += "SHIFT+";
    }

    SDL_Keycode sdlKeyCode = SDL_GetKeyFromScancode(keyCodeToSDLScancode(key));
    str += SDL_GetKeyName(sdlKeyCode);
    size_t len = std::min(sizeof(str) - 1, str.size());
    memcpy(m_szShortcut, str.c_str(), len);
    str[len] = '\0';
}

std::list<KeyBind *> &KeyBind::getList() {
    static std::list<KeyBind *> list;
    return list;
}
