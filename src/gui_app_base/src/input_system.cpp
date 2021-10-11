#include <imgui_impl_sdl.h>
#include <appfw/appfw.h>
#include <gui_app_base/input_system.h>
#include <gui_app_base/gui_app_base.h>

static ConCommand cmd_bind("bind", "Binds a key to a command", [](const CmdString &cmd) {
    if (cmd.size() != 2 && cmd.size() != 3) {
        printi("Usage: bind <key> [command]");
        return;
    }

    SDL_Scancode key = InputSystem::get().getScancodeForKey(cmd[1]);

    if (key == SDL_SCANCODE_UNKNOWN) {
        printe("Unknown key {}", cmd[1]);
        return;
    }

    if (cmd.size() == 2) {
        auto &keyCmd = InputSystem::get().getKeyBind(key);
        if (keyCmd.empty()) {
            printi("Key {} is unbound.", cmd[1]);
        } else {
            CmdString blet; // hack around bogus "the usage of CmdString::toString requires the compiler to capture 'this'" 
            printi("Key {} is bound to {}", cmd[1], blet.toString(keyCmd));
        }

        return;
    }

    InputSystem::get().bindKey(key, CmdString::parse(cmd[2]));
});

static ConCommand cmd_unbind("unbind", "Unbinds a key", [](const CmdString &cmd) {
    if (cmd.size() <= 1) {
        printi("Usage: unbind <key>");
        return;
    }

    SDL_Scancode key = InputSystem::get().getScancodeForKey(cmd[1]);

    if (key == SDL_SCANCODE_UNKNOWN) {
        printe("Unknown key {}", cmd[1]);
        return;
    }

    InputSystem::get().bindKey(key, std::vector<CmdString>());
});

static ConCommand
    cmd_toggleinput("toggleinput", "Enables/disables input grab", [](const CmdString &) {
    // Toggle input
    InputSystem::get().setGrabInput(!InputSystem::get().isInputGrabbed());
});

InputSystem::InputSystem() {
    setTickEnabled(true);
    createScancodeMap();
    setGrabInput(false);

    // Bind a few keys
    bindKey(SDL_SCANCODE_ESCAPE, "toggleinput");
}

InputSystem::~InputSystem() {
}

bool InputSystem::handleSDLEvent(const SDL_Event &event) {
    auto fnProcessImGui = [&]() {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
            return ImGui_ImplSDL2_ProcessEvent(&event);
        } else {
            return false;
        }
    };

    switch (event.type) {
    case SDL_KEYDOWN: {
        auto &keyCmd = m_KeyBinds[event.key.keysym.scancode];
        if (!keyCmd.empty()) {
            for (auto &i : keyCmd) {
                appfw::getConsole().command(i);
            }
        }

        fnProcessImGui();
        return true;
    }
    case SDL_MOUSEMOTION: {
        if (isInputGrabbed()) {
            m_iMouseRelX += event.motion.xrel;
            m_iMouseRelY += event.motion.yrel;
            return true;
        }

        fnProcessImGui();
        return true;
    }
    case SDL_MOUSEBUTTONDOWN: {
        if (isInputGrabbed()) {
            auto &keyCmd = m_MouseBinds[event.button.button];
            if (!keyCmd.empty()) {
                for (auto &i : keyCmd) {
                    appfw::getConsole().command(i);
                }
            }

            return true;
        }

        fnProcessImGui();
        return true;
    }
    case SDL_TEXTINPUT: {
        // HACK: Ignore ` key so it isn't typed in the console
        if (event.text.text[0] == '`' && event.text.text[1] == '\0') {
            return true;
        }

        return fnProcessImGui();
    }
    default: {
        return fnProcessImGui();
    }
    }
}

void InputSystem::tick() {}

bool InputSystem::isInputGrabbed() { return m_bGrabInput; }

void InputSystem::setGrabInput(bool state) {
    if (state) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        // FIXME: ImGui_ImplSDL2_ResetInput(); //!< Unstuck all keys

    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    m_bGrabInput = state;
}

SDL_Scancode InputSystem::getScancodeForKey(const std::string &key) {
    auto it = m_ScancodeMap.find(key);
    if (it == m_ScancodeMap.end()) {
        return SDL_SCANCODE_UNKNOWN;
    } else {
        return it->second;
    }
}

const std::vector<CmdString> &InputSystem::getKeyBind(SDL_Scancode key) {
    if (key < 0) {
        return m_MouseBinds[-key];
    } else {
        return m_KeyBinds[key];
    }
}

void InputSystem::bindKey(SDL_Scancode key, const std::string &cmd) {
    bindKey(key, CmdString::parse(cmd));
}

void InputSystem::bindKey(SDL_Scancode key, const std::vector<CmdString> &cmd) {
    if (key < 0) {
        m_MouseBinds[-key] = cmd;
    } else {
        m_KeyBinds[key] = cmd;
    }
}

void InputSystem::getMouseMovement(int &x, int &y) {
    AFW_ASSERT(isInputGrabbed());
    peekMouseMovement(x, y);
    discardMouseMovement();
}

void InputSystem::peekMouseMovement(int &x, int &y) {
    AFW_ASSERT(isInputGrabbed());
    x = m_iMouseRelX;
    y = m_iMouseRelY;
}

void InputSystem::discardMouseMovement() {
    m_iMouseRelX = 0;
    m_iMouseRelY = 0;
}

void InputSystem::createScancodeMap() {
    // Mouse buttons
    m_ScancodeMap["mouse1"] = (SDL_Scancode)(-SDL_BUTTON_LEFT);
    m_ScancodeMap["mouse2"] = (SDL_Scancode)(-SDL_BUTTON_RIGHT);
    m_ScancodeMap["mouse3"] = (SDL_Scancode)(-SDL_BUTTON_MIDDLE);
    m_ScancodeMap["mouse4"] = (SDL_Scancode)(-SDL_BUTTON_X1);
    m_ScancodeMap["mouse5"] = (SDL_Scancode)(-SDL_BUTTON_X2);

    // Individual keys
    m_ScancodeMap["return"] = SDL_SCANCODE_RETURN;
    m_ScancodeMap["escape"] = SDL_SCANCODE_ESCAPE;
    m_ScancodeMap["backspace"] = SDL_SCANCODE_BACKSPACE;
    m_ScancodeMap["tab"] = SDL_SCANCODE_TAB;
    m_ScancodeMap["space"] = SDL_SCANCODE_SPACE;
    m_ScancodeMap["~"] = SDL_SCANCODE_GRAVE;

    // Key ranges
    std::string oneCharString = "?";

    // A - Z
    for (char c = 'a'; c <= 'z'; c++) {
        SDL_Scancode code = (SDL_Scancode)(SDL_SCANCODE_A + c - 'a');
        oneCharString[0] = c;
        m_ScancodeMap[oneCharString] = code;
    }

    // 1 - 9
    for (char c = '1'; c <= '9'; c++) {
        SDL_Scancode code = (SDL_Scancode)(SDL_SCANCODE_1 + c - '1');
        oneCharString[0] = c;
        m_ScancodeMap[oneCharString] = code;
    }

    // 0
    m_ScancodeMap["0"] = SDL_SCANCODE_0;

    // F1 - F12
    for (int i = 1; i <= 12; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "f%d", i);
        m_ScancodeMap[buf] = (SDL_Scancode)(SDL_SCANCODE_F1 + i - 1);
    }
}
