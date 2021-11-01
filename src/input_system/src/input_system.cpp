#include <imgui_impl_sdl.h>
#include <appfw/appfw.h>
#include <input/input_system.h>
#include <input/key_codes.h>
#include <input/key_bind.h>

#define DEBUG_BINDS 1

#if DEBUG_BINDS
static KeyBind testBinds[] = {
    KeyBind("test1", KeyCode::F),
    KeyBind("test2", KeyCode::A),
    KeyBind("test3", KeyCode::A, KMOD_CTRL),
    KeyBind("test4", KeyCode::A, KMOD_CTRL | KMOD_ALT),
};
#endif

static uint8_t s_ReleasedKeyArray[SDL_NUM_SCANCODES] = {0};

InputSystem::InputSystem() {
    memset(m_KeyboardStateThisFrame, 0, sizeof(m_KeyboardStateThisFrame));
    setTickEnabled(true);

    int numKeys = 0;
    m_pKeyboardState = SDL_GetKeyboardState(&numKeys);
    AFW_ASSERT_REL(numKeys >= SDL_NUM_SCANCODES);
   
    // Init all key binds
    auto &binds = KeyBind::getList();

    for (KeyBind *bind : binds) {
        KeyCode key = bind->getDefaultKeyCode();
        int mods = bind->getDefaultModKeys();
        bind->setKeyCode(key, mods); // This can't be done in constructor because it calls SDL which
                                     // isn't initialized until main()
    }
}

InputSystem::~InputSystem() {
}

void InputSystem::beginTick() {
    // Release all keys held in the prev frame
    memset(m_KeyboardStateThisFrame, 0, sizeof(m_KeyboardStateThisFrame));

    // See if ImGui wants keyboard input
    ImGuiIO &imio = ImGui::GetIO();
    bool imguiKeyboardControl = imio.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard;
    bool passKeyboardToImGui = imguiKeyboardControl ? imio.WantTextInput : imio.WantCaptureKeyboard;
    m_bImGuiWantsInput = passKeyboardToImGui;

    if (passKeyboardToImGui || m_bGrabInput) {
        // Release all keys
        m_pKeyboardState = s_ReleasedKeyArray;
        m_KeyboardMods = 0;
    } else {
        m_pKeyboardState = SDL_GetKeyboardState(nullptr);
    }
}

bool InputSystem::handleSDLEvent(const SDL_Event &event) {
    if (m_bGrabInput) {
        // Filter some event from ImGui
        switch (event.type) {
        case SDL_MOUSEMOTION: {
            m_iMouseRelX += event.motion.xrel;
            m_iMouseRelY += event.motion.yrel;
            return true;
        }
        case SDL_MOUSEWHEEL:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_TEXTINPUT:
        case SDL_KEYDOWN:
            // Ignore
            return false;
        default: {
            // Pass to ImGui
            return ImGui_ImplSDL2_ProcessEvent(&event);
        }
        }
    } else if (m_bImGuiWantsInput) {
        // Let ImGui handle everything
        return ImGui_ImplSDL2_ProcessEvent(&event);
    } else {
        // Update keys
        m_KeyboardMods = SDL_GetModState() & SUPPORTED_KMOD;

        // Handle events
        switch (event.type) {
        case SDL_KEYDOWN: {
            m_KeyboardStateThisFrame[event.key.keysym.scancode] = true;
            return true;
        }
        case SDL_TEXTINPUT:
            // Ignore
            return false;
        default: {
            // Pass to ImGui
            return ImGui_ImplSDL2_ProcessEvent(&event);
        }
        }
    }
}

void InputSystem::tick() {
    ImGui::Begin("Input");
    ImGui::Text("ImGui takes keyboard: %d", (int)m_bImGuiWantsInput);

#if DEBUG_BINDS
    for (int i = 0; i < std::size(testBinds); i++) {
        KeyBind &bind = testBinds[i];

        if (bind.isPressed()) {
            printd("Pressed: {}", bind.getShortcut());
        }

        if (bind.isHeldDown()) {
            ImGui::Text("Bind %s", bind.getShortcut());
        }
    }
#endif

    ImGui::End();
}

void InputSystem::setGrabInput(bool state) {
    if (state) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    m_bGrabInput = state;
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
